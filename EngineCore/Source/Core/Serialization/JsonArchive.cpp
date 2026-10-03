#include "SimpleEngine/Core/Serialization/JsonArchive.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/Stack.h"
#include "SimpleEngine/Utility/Base64.h"
#include "SimpleEngine/Utility/Overloaded.h"

#include "Core/Serialization/TextArchiveCommon.h"

#include <yyjson.h>

#include <algorithm>
#include <charconv>
#include <compare>
#include <limits>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>


namespace se
{
namespace
{
/** JSON 숫자로 정확히 표현할 수 있는 정수의 최대 절댓값 (2^53 - 1) */
constexpr i64 MAX_SAFE_INTEGER = 9007199254740991;

/** 출력의 들여쓰기 한 단계에 쓰는 공백 수 */
constexpr usize INDENT_WIDTH = 4;

/** 정수 노드의 값을 JSON 숫자로 쓸 수 있는지 확인합니다. 절댓값이 2^53 - 1을 넘으면 10진 문자열로 써야 합니다. */
[[nodiscard]] bool FitsJsonNumber(i64 value, EIntWidth width, bool is_signed)
{
    // i64를 넘는 u64는 비트가 음수 i64로 들어옴
    if (width == EIntWidth::Bits64 && !is_signed)
    {
        return static_cast<u64>(value) <= static_cast<u64>(MAX_SAFE_INTEGER);
    }
    return value >= -MAX_SAFE_INTEGER && value <= MAX_SAFE_INTEGER;
}

/** 정수 노드의 값을 부호에 맞는 10진 문자열로 바꿉니다. */
[[nodiscard]] String ToDecimalText(i64 value, EIntWidth width, bool is_signed)
{
    if (width == EIntWidth::Bits64 && !is_signed)
    {
        return String::Format("{}", static_cast<u64>(value));
    }
    return String::Format("{}", value);
}

/** 오류 메시지에 쓸 JSON 값 종류의 이름을 돌려줍니다. */
[[nodiscard]] StringView JsonKindName(yyjson_val* value)
{
    if (yyjson_is_obj(value))  { return "an object";  }
    if (yyjson_is_arr(value))  { return "an array";   }
    if (yyjson_is_str(value))  { return "a string";   }
    if (yyjson_is_int(value))  { return "an integer"; }
    if (yyjson_is_real(value)) { return "a float";    }
    if (yyjson_is_bool(value)) { return "a boolean";  }
    if (yyjson_is_null(value)) { return "null";       }
    return "an unknown value";
}

/** 배열의 원소를 차례로 모아 돌려줍니다. */
[[nodiscard]] Array<yyjson_mut_val*> ElementsOf(yyjson_mut_val* array)
{
    Array<yyjson_mut_val*> elements;
    elements.Reserve(yyjson_mut_arr_size(array));

    yyjson_mut_arr_iter iter;
    yyjson_mut_arr_iter_init(array, &iter);
    while (yyjson_mut_val* const element = yyjson_mut_arr_iter_next(&iter))
    {
        elements.Push(element);
    }
    return elements;
}

/** 문자열 값의 내용을 돌려줍니다. */
[[nodiscard]] std::string_view TextOf(yyjson_mut_val* value)
{
    return { yyjson_mut_get_str(value), yyjson_mut_get_len(value) };
}

/** 숫자, 문자열, bool처럼 컨테이너가 아닌 값인지 확인합니다. */
[[nodiscard]] bool IsScalar(yyjson_mut_val* value)
{
    return yyjson_mut_is_num(value) || yyjson_mut_is_str(value) || yyjson_mut_is_bool(value);
}

/** 객체의 모든 멤버 값이 스칼라인지 확인합니다. */
[[nodiscard]] bool HasOnlyScalarMembers(yyjson_mut_val* object)
{
    yyjson_mut_obj_iter iter;
    yyjson_mut_obj_iter_init(object, &iter);
    while (yyjson_mut_val* const key = yyjson_mut_obj_iter_next(&iter))
    {
        if (!IsScalar(yyjson_mut_obj_iter_get_val(key)))
        {
            return false;
        }
    }
    return true;
}

/** 들여쓰기 depth 단계만큼 공백을 붙입니다. */
void AppendIndent(String& out, usize depth)
{
    constexpr StringView INDENT = "    ";
    static_assert(INDENT.ByteLen() == INDENT_WIDTH);

    for (usize level = 0; level < depth; ++level)
    {
        out.Append(INDENT);
    }
}

/** byte의 짧은 JSON 이스케이프를 돌려줍니다. 없으면 빈 문자열입니다. */
[[nodiscard]] StringView ShortEscapeOf(u8 byte)
{
    switch (byte)
    {
    case '"':  return "\\\"";
    case '\\': return "\\\\";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\t': return "\\t";
    default:   return {};
    }
}

/** text를 큰따옴표로 감싸고 JSON 이스케이프를 적용해 붙입니다. 0x20 이상의 바이트(UTF-8 포함)는 그대로 씁니다. */
void AppendQuoted(String& out, StringView text)
{
    constexpr StringView HEX_DIGITS = "0123456789abcdef";

    out.Append("\"");

    // 이스케이프할 바이트는 모두 ASCII라서, 그 사이의 구간은 UTF-8 글자 경계에서 잘려 그대로 붙일 수 있음
    usize run_begin = 0;
    for (usize index = 0; index < text.ByteLen(); ++index)
    {
        const u8 byte = static_cast<u8>(text.Data()[index]);
        const StringView escape = ShortEscapeOf(byte);
        if (escape.IsEmpty() && byte >= 0x20)
        {
            continue;
        }

        out.Append(text.Substr(run_begin, index - run_begin));
        if (!escape.IsEmpty())
        {
            out.Append(escape);
        }
        else
        {
            // NOLINTBEGIN(*-signed-bitwise)
            const char hex[] = { HEX_DIGITS.Data()[byte >> 4], HEX_DIGITS.Data()[byte & 0x0F] };
            // NOLINTEND(*-signed-bitwise)
            out.Append("\\u00");
            out.Append(StringView{ hex, 2 });
        }
        run_begin = index + 1;
    }
    out.Append(text.Substr(run_begin));

    out.Append("\"");
}

/** 정수를 10진으로 붙입니다. */
template <traits::IntegralType T>
void AppendInteger(String& out, T value)
{
    char buffer[24];
    const char* const end = std::to_chars(std::begin(buffer), std::end(buffer), value).ptr;
    out.Append(StringView{ buffer, static_cast<usize>(end - buffer) });
}

/** 실수를 최단 표현으로 붙입니다. 정수와 구분되도록 소수점이나 지수가 없으면 ".0"을 붙입니다(1은 1.0). */
void AppendReal(String& out, f64 value)
{
    char buffer[32];
    [[maybe_unused]] const auto [end, error] = std::to_chars(std::begin(buffer), std::end(buffer), value);
    SE_ASSERT(error == std::errc{}, "a finite f64 always fits in 32 characters");

    const std::string_view text(buffer, end);
    out.Append(StringView{ text });
    if (text.find_first_of(".eE") == std::string_view::npos)
    {
        out.Append(".0");
    }
}

/**
 * value를 출력 모양에 맞춰 붙입니다. depth는 value가 놓인 들여쓰기 단계입니다.
 * 스칼라만 담은 비어 있지 않은 객체는 한 줄이고, 그 밖의 객체와 배열은 멤버나 원소마다 한 줄입니다.
 */
void AppendValue(String& out, yyjson_mut_val* value, usize depth) // NOLINT(*-no-recursion)
{
    if (yyjson_mut_is_str(value))
    {
        AppendQuoted(out, TextOf(value));
        return;
    }
    if (yyjson_mut_is_bool(value))
    {
        out.Append(yyjson_mut_is_true(value) ? "true" : "false");
        return;
    }
    if (yyjson_mut_is_sint(value))
    {
        AppendInteger(out, yyjson_mut_get_sint(value));
        return;
    }
    if (yyjson_mut_is_uint(value))
    {
        AppendInteger(out, yyjson_mut_get_uint(value));
        return;
    }
    if (yyjson_mut_is_real(value))
    {
        AppendReal(out, yyjson_mut_get_real(value));
        return;
    }

    if (yyjson_mut_is_arr(value))
    {
        if (yyjson_mut_arr_size(value) == 0)
        {
            out.Append("[]");
            return;
        }

        out.Append("[\n");
        yyjson_mut_arr_iter iter;
        yyjson_mut_arr_iter_init(value, &iter);
        bool is_first = true;
        while (yyjson_mut_val* const element = yyjson_mut_arr_iter_next(&iter))
        {
            if (!is_first)
            {
                out.Append(",\n");
            }
            is_first = false;
            AppendIndent(out, depth + 1);
            AppendValue(out, element, depth + 1);
        }
        out.Append("\n");
        AppendIndent(out, depth);
        out.Append("]");
        return;
    }

    SE_ASSERT(yyjson_mut_is_obj(value), "the writer only creates strings, numbers, booleans, arrays, and objects");
    if (yyjson_mut_obj_size(value) == 0)
    {
        out.Append("{}");
        return;
    }

    const bool is_one_line = HasOnlyScalarMembers(value);
    out.Append(is_one_line ? "{ " : "{\n");
    yyjson_mut_obj_iter iter;
    yyjson_mut_obj_iter_init(value, &iter);
    bool is_first = true;
    while (yyjson_mut_val* const key = yyjson_mut_obj_iter_next(&iter))
    {
        if (!is_first)
        {
            out.Append(is_one_line ? ", " : ",\n");
        }
        is_first = false;
        if (!is_one_line)
        {
            AppendIndent(out, depth + 1);
        }
        AppendQuoted(out, TextOf(key));
        out.Append(": ");
        AppendValue(out, yyjson_mut_obj_iter_get_val(key), depth + 1);
    }
    if (is_one_line)
    {
        out.Append(" }");
        return;
    }
    out.Append("\n");
    AppendIndent(out, depth);
    out.Append("}");
}

/** 값 종류의 비교 순서. 종류가 다른 값은 이 순서로 정렬합니다. */
enum class EValueKind : u8
{
    Bool,
    Number,
    String,
    Array,
    Object,
};

/** value의 종류를 돌려줍니다. */
[[nodiscard]] EValueKind KindOf(yyjson_mut_val* value)
{
    if (yyjson_mut_is_bool(value)) { return EValueKind::Bool; }
    if (yyjson_mut_is_num(value))  { return EValueKind::Number; }
    if (yyjson_mut_is_str(value))  { return EValueKind::String; }
    if (yyjson_mut_is_arr(value))  { return EValueKind::Array; }
    SE_ASSERT(yyjson_mut_is_obj(value), "the writer only creates strings, numbers, booleans, arrays, and objects");
    return EValueKind::Object;
}

/** 두 정수 값을 부호와 크기까지 정확하게 비교합니다. i64는 sint, 그 위의 u64는 uint로 저장되어 있습니다. */
[[nodiscard]] std::strong_ordering CompareIntegers(yyjson_mut_val* lhs, yyjson_mut_val* rhs)
{
    const bool lhs_is_signed = yyjson_mut_is_sint(lhs);
    const bool rhs_is_signed = yyjson_mut_is_sint(rhs);
    if (lhs_is_signed && rhs_is_signed)
    {
        return yyjson_mut_get_sint(lhs) <=> yyjson_mut_get_sint(rhs);
    }
    if (!lhs_is_signed && !rhs_is_signed)
    {
        return yyjson_mut_get_uint(lhs) <=> yyjson_mut_get_uint(rhs);
    }

    // 한쪽만 부호 있는 값이면, 음수는 항상 작고 아니면 u64로 맞춰 비교
    const i64 signed_value = lhs_is_signed ? yyjson_mut_get_sint(lhs) : yyjson_mut_get_sint(rhs);
    const u64 unsigned_value = lhs_is_signed ? yyjson_mut_get_uint(rhs) : yyjson_mut_get_uint(lhs);
    const std::strong_ordering signed_side = signed_value < 0
        ? std::strong_ordering::less
        : static_cast<u64>(signed_value) <=> unsigned_value;
    return lhs_is_signed ? signed_side : 0 <=> signed_side;
}

/**
 * 순서 없는 출력을 정렬할 때 쓰는 전순서로 두 값을 비교합니다.
 * 종류가 다르면 EValueKind 순서이고, 같으면 숫자는 값, 문자열은 바이트 사전순, bool은 false가 먼저입니다.
 * 배열은 원소를 앞에서부터 비교하고([key, value] 쌍은 key가 먼저), 객체는 출력 텍스트로 비교합니다.
 */
[[nodiscard]] std::weak_ordering CompareValues(yyjson_mut_val* lhs, yyjson_mut_val* rhs) // NOLINT(*-no-recursion)
{
    const EValueKind lhs_kind = KindOf(lhs);
    const EValueKind rhs_kind = KindOf(rhs);
    if (lhs_kind != rhs_kind)
    {
        return lhs_kind <=> rhs_kind;
    }

    switch (lhs_kind)
    {
    case EValueKind::Bool:
        return yyjson_mut_is_true(lhs) <=> yyjson_mut_is_true(rhs);
    case EValueKind::Number:
        if (yyjson_mut_is_int(lhs) && yyjson_mut_is_int(rhs))
        {
            return CompareIntegers(lhs, rhs);
        }
        return std::strong_order(yyjson_mut_get_num(lhs), yyjson_mut_get_num(rhs));
    case EValueKind::String:
        return TextOf(lhs) <=> TextOf(rhs);
    case EValueKind::Array:
    {
        yyjson_mut_arr_iter lhs_iter;
        yyjson_mut_arr_iter rhs_iter;
        yyjson_mut_arr_iter_init(lhs, &lhs_iter);
        yyjson_mut_arr_iter_init(rhs, &rhs_iter);
        while (true)
        {
            yyjson_mut_val* const lhs_element = yyjson_mut_arr_iter_next(&lhs_iter);
            yyjson_mut_val* const rhs_element = yyjson_mut_arr_iter_next(&rhs_iter);

            // 한쪽이 먼저 끝나면 짧은 쪽이 앞
            if (lhs_element == nullptr || rhs_element == nullptr)
            {
                return (lhs_element != nullptr) <=> (rhs_element != nullptr);
            }
            if (const std::weak_ordering order = CompareValues(lhs_element, rhs_element); order != 0)
            {
                return order;
            }
        }
    }
    case EValueKind::Object:
    {
        String lhs_text;
        String rhs_text;
        AppendValue(lhs_text, lhs, 0);
        AppendValue(rhs_text, rhs, 0);
        return lhs_text <=> rhs_text;
    }
    }
    SE_UNREACHABLE();
}

/**
 * value 안의 모든 객체에서 같은 키가 두 번 나오는지 찾아, 처음 발견한 것의 오류 메시지를 돌려줍니다.
 * path는 value의 문서 안 위치이고 중첩이 serde::MAX_NESTING_DEPTH를 넘으면 오류입니다. 없으면 NullOpt를 돌려줍니다.
 */
[[nodiscard]] Optional<String> FindDuplicateKey(yyjson_val* value, String& path, usize depth) // NOLINT(*-no-recursion)
{
    if (!yyjson_is_ctn(value))
    {
        return NullOpt;
    }
    if (depth > serde::MAX_NESTING_DEPTH)
    {
        return String::Format("JsonReader: the document is nested deeper than {} levels.", serde::MAX_NESTING_DEPTH);
    }

    const usize parent_length = path.ByteLen();
    if (yyjson_is_arr(value))
    {
        yyjson_arr_iter iter;
        yyjson_arr_iter_init(value, &iter);
        while (yyjson_val* const element = yyjson_arr_iter_next(&iter))
        {
            // 방금 꺼낸 원소의 번호는 iter.idx - 1
            path.Append(String::Format("[{}]", iter.idx - 1));
            if (auto error = FindDuplicateKey(element, path, depth + 1))
            {
                return error;
            }
            path.Truncate(parent_length);
        }
        return NullOpt;
    }

    std::unordered_set<std::string_view> seen_keys;
    seen_keys.reserve(yyjson_obj_size(value));

    yyjson_obj_iter iter;
    yyjson_obj_iter_init(value, &iter);
    while (yyjson_val* const key = yyjson_obj_iter_next(&iter))
    {
        const StringView name = std::string_view{ yyjson_get_str(key), yyjson_get_len(key) };
        if (!seen_keys.insert(std::string_view{ name }).second)
        {
            const StringView location = path.IsEmpty() ? StringView{ "(root)" } : StringView{ path };
            return String::Format("JsonReader: duplicate key '{}' at '{}'.", name, location);
        }

        if (!path.IsEmpty())
        {
            path.Append(".");
        }
        path.Append(name);
        if (auto error = FindDuplicateKey(yyjson_obj_iter_get_val(key), path, depth + 1))
        {
            return error;
        }
        path.Truncate(parent_length);
    }
    return NullOpt;
}
} // namespace


// JsonWriter
struct JsonWriter::State
{
    /** Field가 정한 키로 값을 넣는 struct의 객체 */
    struct StructFrame
    {
        yyjson_mut_val* object = nullptr;

        /** Field가 정한, 다음 값을 넣을 키 */
        Optional<String> pending_key;

        /** pending_key 자리에 값이 있는 Optional을 열었는지 여부 */
        bool pending_key_in_some = false;
    };

    /** 끝에 값을 넣는 시퀀스의 배열 */
    struct SeqFrame
    {
        yyjson_mut_val* array = nullptr;

        /** 순서 없는 시퀀스는 EndSeq에서 정렬합니다. */
        ESeqOrder order = ESeqOrder::Ordered;
    };

    /**
     * EndMap에서 객체나 쌍 배열로 만들 맵의 엔트리 모음
     * key를 모두 봐야 객체로 쓸 수 있는지 알 수 있어 EndMap까지 모읍니다.
     */
    struct MapFrame
    {
        /** 지금까지 쓴 [key, value] 배열의 배열 */
        yyjson_mut_val* entries = nullptr;
    };

    /** key와 value를 차례로 넣는 맵 엔트리의 [key, value] 배열 */
    struct MapEntryFrame
    {
        yyjson_mut_val* pair = nullptr;
    };

    /**
     * 쓰는 중인 컨테이너 하나
     * @note std::variant의 operator<는 제약 없이 선언되어 Deque의 기본 operator<=>가 컴파일되지 않으므로, 구조체로 감쌉니다.
     */
    struct OpenContainer
    {
        std::variant<StructFrame, SeqFrame, MapFrame, MapEntryFrame> frame;
    };

    explicit State(Archive& in_archive)
        : archive(in_archive)
        , doc(yyjson_mut_doc_new(nullptr))
    {
        SE_ASSERT(doc != nullptr, "yyjson failed to allocate a document");
    }

    ~State()
    {
        yyjson_mut_doc_free(doc);
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;

    /** text를 문서에 복사한 문자열 값을 만듭니다. 문서가 소유하므로 text의 수명과 무관합니다. */
    [[nodiscard]] yyjson_mut_val* MakeString(StringView text) const
    {
        return yyjson_mut_strncpy(doc, text.IsEmpty() ? "" : text.Data(), text.ByteLen());
    }

    /**
     * 지금 열린 컨테이너(없으면 루트)에 값 하나를 넣을 수 있는지 확인합니다.
     * struct면 Field로 키를 정했어야 하고, 맵이면 BeginMapEntry로 엔트리를 열었어야 합니다. 넣을 수 없으면 SetError를 호출하고 false를 돌려줍니다.
     */
    [[nodiscard]] bool CanPlaceValue()
    {
        if (archive.HasError())
        {
            return false;
        }

        if (open_containers.IsEmpty())
        {
            if (yyjson_mut_doc_get_root(doc) != nullptr)
            {
                archive.SetError("JsonWriter: the document already has a root value.");
                return false;
            }
            return true;
        }
        if (const auto frame = TopAs<StructFrame>(); frame && !frame->pending_key)
        {
            archive.SetError("JsonWriter: a value inside a struct needs a Field name first.");
            return false;
        }
        if (TopAs<MapFrame>())
        {
            archive.SetError("JsonWriter: a value inside a map needs BeginMapEntry first.");
            return false;
        }
        return true;
    }

    /**
     * value를 지금 열린 컨테이너에 넣습니다. struct면 Field가 정한 키로, 시퀀스와 맵 엔트리면 배열 끝에, 비었으면 루트로 넣습니다.
     * 넣을 곳이 없으면 SetError를 호출하고 false를 돌려줍니다.
     */
    bool PlaceValue(yyjson_mut_val* value)
    {
        if (!CanPlaceValue())
        {
            return false;
        }

        bool is_placed = false;
        if (const auto struct_frame = TopAs<StructFrame>())
        {
            is_placed = yyjson_mut_obj_add(struct_frame->object, MakeString(*struct_frame->pending_key), value);
            struct_frame->pending_key.Reset();
            struct_frame->pending_key_in_some = false;
        }
        else if (const auto seq_frame = TopAs<SeqFrame>())
        {
            is_placed = yyjson_mut_arr_append(seq_frame->array, value);
        }
        else if (const auto entry_frame = TopAs<MapEntryFrame>())
        {
            is_placed = yyjson_mut_arr_append(entry_frame->pair, value);
        }
        else
        {
            // 루트 값 (CanPlaceValue가 두 번째 루트 값을 거절함)
            is_placed = value != nullptr;
            yyjson_mut_doc_set_root(doc, value);
        }

        if (!is_placed)
        {
            archive.SetError("JsonWriter: failed to allocate a JSON value.");
        }
        return is_placed;
    }

    /** 정수 노드의 값을 넣습니다. 절댓값이 2^53 - 1을 넘으면 10진 문자열로 넣습니다. */
    void PlaceInteger(i64 value, EIntWidth width, bool is_signed)
    {
        if (!FitsJsonNumber(value, width, is_signed))
        {
            PlaceValue(MakeString(ToDecimalText(value, width, is_signed)));
            return;
        }
        PlaceValue(is_signed ? yyjson_mut_sint(doc, value) : yyjson_mut_uint(doc, static_cast<u64>(value)));
    }

    /** 맨 위 컨테이너가 Frame이면 그 프레임을, 비었거나 다른 종류면 NullOpt를 돌려줍니다. */
    template <typename Frame>
    [[nodiscard]] Optional<Frame&> TopAs()
    {
        const auto top = open_containers.Peek();
        if (!top)
        {
            return NullOpt;
        }
        if (Frame* const frame = std::get_if<Frame>(&top->frame))
        {
            return *frame;
        }
        return NullOpt;
    }

    Archive& archive;
    yyjson_mut_doc* doc = nullptr;
    Stack<OpenContainer> open_containers;
};

JsonWriter::JsonWriter()
    : state(std::make_unique<State>(*this))
{
}

JsonWriter::~JsonWriter() = default;

Expected<String, String> JsonWriter::ToText() const
{
    if (HasError())
    {
        return Unexpected{ String(GetError()) };
    }
    if (!state->open_containers.IsEmpty())
    {
        return Unexpected{ "JsonWriter: the document has an unclosed container." };
    }
    yyjson_mut_val* const root = yyjson_mut_doc_get_root(state->doc);
    if (root == nullptr)
    {
        return Unexpected{ "JsonWriter: the document has no value." };
    }

    String text;
    AppendValue(text, root, 0);
    text.Append("\n");
    return text;
}

bool JsonWriter::IsTextFormat() const
{
    return true;
}

bool JsonWriter::SupportsRawElements() const
{
    return false;
}

void JsonWriter::Int(i64 value, EIntWidth width, bool is_signed)
{
    state->PlaceInteger(value, width, is_signed);
}

void JsonWriter::Float(f64 value, EFloatWidth width)
{
    const f64 number = width == EFloatWidth::Bits32 ? text_archive::ToShortestF64(static_cast<f32>(value)) : value;
    if (!std::isfinite(number))
    {
        SetError("JsonWriter: NaN and infinity cannot be written in JSON.");
        return;
    }
    state->PlaceValue(yyjson_mut_real(state->doc, number));
}

void JsonWriter::Bool(bool value)
{
    state->PlaceValue(yyjson_mut_bool(state->doc, value));
}

void JsonWriter::Str(StringView value)
{
    state->PlaceValue(state->MakeString(value));
}

void JsonWriter::Bytes(const void* data, u64 size)
{
    const String text = base64::Encode(ArrayView<const u8>(static_cast<const u8*>(data), size));
    state->PlaceValue(state->MakeString(text));
}

void JsonWriter::RawElements([[maybe_unused]] const void* data, [[maybe_unused]] u64 size)
{
    SetError("JsonWriter: raw element bytes are not supported in a text format.");
}

void JsonWriter::Enum(i64 value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries)
{
    // 이름이 있는 값은 이름으로, 없는 값(플래그 조합 등)은 정수로 씀
    const auto* const entry = std::ranges::find(entries, value, &EnumEntry::value);
    if (entry != entries.end())
    {
        state->PlaceValue(state->MakeString(entry->name));
        return;
    }

    // Int처럼 문자열로 쓰면 이름으로 읽혀 되읽을 수 없으므로 오류
    if (!FitsJsonNumber(value, width, is_signed))
    {
        SetError(String::Format("JsonWriter: enum value {} has no name and does not fit in a JSON number.", ToDecimalText(value, width, is_signed)));
        return;
    }
    state->PlaceInteger(value, width, is_signed);
}

void JsonWriter::BeginStruct()
{
    yyjson_mut_val* const object = yyjson_mut_obj(state->doc);
    if (state->PlaceValue(object))
    {
        state->open_containers.Push({ State::StructFrame{ .object = object } });
    }
}

void JsonWriter::Field(StringView name)
{
    if (HasError())
    {
        return;
    }

    const auto frame = state->TopAs<State::StructFrame>();
    if (!frame)
    {
        SetError(String::Format("JsonWriter: field '{}' is outside a struct.", name));
        return;
    }
    if (frame->pending_key)
    {
        SetError(String::Format("JsonWriter: field '{}' has no value.", *frame->pending_key));
        return;
    }
    frame->pending_key = String(name);
}

void JsonWriter::EndStruct()
{
    if (HasError())
    {
        return;
    }

    const auto frame = state->TopAs<State::StructFrame>();
    if (!frame)
    {
        SetError("JsonWriter: EndStruct does not match an open struct.");
        return;
    }
    if (frame->pending_key)
    {
        SetError(String::Format("JsonWriter: field '{}' has no value.", *frame->pending_key));
        return;
    }
    state->open_containers.Pop();
}

void JsonWriter::BeginSeq([[maybe_unused]] u64 count, ESeqOrder order)
{
    yyjson_mut_val* const array = yyjson_mut_arr(state->doc);
    if (state->PlaceValue(array))
    {
        state->open_containers.Push({ State::SeqFrame{ .array = array, .order = order } });
    }
}

void JsonWriter::EndSeq()
{
    if (HasError())
    {
        return;
    }

    const auto frame = state->TopAs<State::SeqFrame>();
    if (!frame)
    {
        SetError("JsonWriter: EndSeq does not match an open sequence.");
        return;
    }

    // 순서 없는 시퀀스는 같은 값이면 같은 텍스트가 나오도록 정렬
    if (frame->order == ESeqOrder::Unordered)
    {
        Array<yyjson_mut_val*> elements = ElementsOf(frame->array);
        elements.Sort([](yyjson_mut_val* lhs, yyjson_mut_val* rhs)
        {
            return CompareValues(lhs, rhs) < 0;
        });

        // 비운 배열에 정렬한 순서로 다시 넣음
        yyjson_mut_arr_clear(frame->array);
        for (yyjson_mut_val* const element : elements)
        {
            yyjson_mut_arr_append(frame->array, element);
        }
    }
    state->open_containers.Pop();
}

void JsonWriter::BeginMap([[maybe_unused]] u64 count)
{
    // 객체로 쓸 수 있는지는 key를 모두 봐야 알 수 있으므로, 넣을 자리만 확인하고 EndMap까지 엔트리를 모아 둠
    if (state->CanPlaceValue())
    {
        state->open_containers.Push({ State::MapFrame{ .entries = yyjson_mut_arr(state->doc) } });
    }
}

void JsonWriter::BeginMapEntry()
{
    if (HasError())
    {
        return;
    }

    const auto frame = state->TopAs<State::MapFrame>();
    if (!frame)
    {
        SetError("JsonWriter: BeginMapEntry is outside a map.");
        return;
    }

    // key와 value를 차례로 받을 [key, value] 배열
    yyjson_mut_val* const pair = yyjson_mut_arr(state->doc);
    if (!yyjson_mut_arr_append(frame->entries, pair))
    {
        SetError("JsonWriter: failed to allocate a JSON value.");
        return;
    }
    state->open_containers.Push({ State::MapEntryFrame{ .pair = pair } });
}

void JsonWriter::EndMapEntry()
{
    if (HasError())
    {
        return;
    }

    const auto frame = state->TopAs<State::MapEntryFrame>();
    if (!frame)
    {
        SetError("JsonWriter: EndMapEntry does not match an open map entry.");
        return;
    }
    if (yyjson_mut_arr_size(frame->pair) != 2)
    {
        SetError("JsonWriter: a map entry needs exactly one key and one value.");
        return;
    }
    state->open_containers.Pop();
}

void JsonWriter::EndMap()
{
    if (HasError())
    {
        return;
    }

    const auto frame = state->TopAs<State::MapFrame>();
    if (!frame)
    {
        SetError("JsonWriter: EndMap does not match an open map.");
        return;
    }
    Array<yyjson_mut_val*> entries = ElementsOf(frame->entries);
    state->open_containers.Pop();

    // key가 모두 문자열이면 객체 (빈 맵 포함)
    const bool has_only_string_keys = std::ranges::all_of(entries, [](yyjson_mut_val* entry)
    {
        return yyjson_mut_is_str(yyjson_mut_arr_get_first(entry));
    });
    if (has_only_string_keys)
    {
        const auto key_of = [](yyjson_mut_val* entry)
        {
            return TextOf(yyjson_mut_arr_get_first(entry));
        };
        entries.Sort([&](yyjson_mut_val* lhs, yyjson_mut_val* rhs)
        {
            return key_of(lhs) < key_of(rhs);
        });

        // 서로 다른 key가 같은 문자열로 쓰이면 한쪽이 사라지므로 오류로 처리
        for (usize index = 1; index < entries.Len(); ++index)
        {
            if (key_of(entries[index - 1]) == key_of(entries[index]))
            {
                SetError(String::Format("JsonWriter: map key '{}' is written twice.", key_of(entries[index])));
                return;
            }
        }

        yyjson_mut_val* const object = yyjson_mut_obj(state->doc);
        for (yyjson_mut_val* const entry : entries)
        {
            // 쌍 배열은 더 쓰지 않으므로 value 노드를 그대로 옮겨 담음. 추가하기 전에 key와 value를 먼저 꺼냄
            yyjson_mut_val* const key = state->MakeString(StringView{ key_of(entry) });
            yyjson_mut_val* const value = yyjson_mut_arr_get_last(entry);
            yyjson_mut_obj_add(object, key, value);
        }
        state->PlaceValue(object);
        return;
    }

    // 아니면 key 순서(key가 같으면 value 순서)로 정렬한 [key, value] 쌍 배열
    entries.Sort([](yyjson_mut_val* lhs, yyjson_mut_val* rhs)
    {
        return CompareValues(lhs, rhs) < 0;
    });
    yyjson_mut_val* const pairs = yyjson_mut_arr(state->doc);
    for (yyjson_mut_val* const entry : entries)
    {
        yyjson_mut_arr_append(pairs, entry);
    }
    state->PlaceValue(pairs);
}

void JsonWriter::Present(bool has_value)
{
    if (HasError())
    {
        return;
    }

    const auto frame = state->TopAs<State::StructFrame>();
    const bool is_field_value = frame && frame->pending_key;
    if (has_value)
    {
        // 이어서 쓰는 내부 값이 이 자리에 들어감
        if (is_field_value)
        {
            frame->pending_key_in_some = true;
        }
        return;
    }

    // None은 struct 필드의 키를 생략해서만 쓸 수 있음. 시퀀스 원소, 맵의 key와 value를 생략하면 그 자리가 사라짐
    if (!is_field_value)
    {
        SetError("JsonWriter: None can only be written as a struct field, by omitting its key.");
        return;
    }

    // Optional<Optional<T>>의 Some(None)은 키를 생략하면 바깥 None으로 읽힘
    if (frame->pending_key_in_some)
    {
        SetError("JsonWriter: None inside another Optional cannot be written because the omitted key reads back as the outer None.");
        return;
    }
    frame->pending_key.Reset();
}

// 텍스트는 값의 경계가 문서 구조에 드러나므로 구간에 쓸 것이 없음
void JsonWriter::BeginSection() {}
void JsonWriter::EndSection() {}


// JsonReader
struct JsonReader::State
{
    /** Field가 찾아 둔 값을 꺼내는 struct의 객체 */
    struct StructFrame
    {
        yyjson_val* object = nullptr;

        /** 문서 안의 위치. 루트는 빈 문자열입니다. */
        String path;

        /** Field가 찾아 둔 다음 값 */
        yyjson_val* field_value = nullptr;

        /** 타입이 물어본 필드 이름 */
        Array<String> known_keys;
    };

    /** 원소를 차례로 꺼내는 시퀀스의 배열 */
    struct SeqFrame
    {
        yyjson_val* array = nullptr;

        /** 문서 안의 위치 */
        String path;

        /** 다음에 꺼낼 원소를 가리키는 반복자. idx가 지금까지 꺼낸 원소 수입니다. */
        yyjson_arr_iter next_element{};
    };

    /** 객체로 적힌 맵 */
    struct ObjectMapFrame
    {
        yyjson_val* object = nullptr;

        /** 문서 안의 위치 */
        String path;

        /** 다음에 열 엔트리를 가리키는 반복자 */
        yyjson_obj_iter next_entry{};
    };

    /** [key, value] 쌍 배열로 적힌 맵 */
    struct PairMapFrame
    {
        yyjson_val* pairs = nullptr;

        /** 문서 안의 위치 */
        String path;

        /** 다음에 열 쌍을 가리키는 반복자 */
        yyjson_arr_iter next_pair{};
    };

    /** 객체 맵의 엔트리 하나 */
    struct ObjectEntryFrame
    {
        /** 키를 담은 문자열 값. key 자리에서 문자열이나 enum 이름으로 읽힙니다. */
        yyjson_val* key = nullptr;

        yyjson_val* value = nullptr;

        /** key와 value가 함께 쓰는 엔트리의 위치 (예: "scores.alice") */
        String path;

        /** 다음에 꺼낼 것 (0은 key, 1은 value) */
        usize next_slot = 0;
    };

    /** 쌍 배열 맵의 엔트리 하나 */
    struct PairEntryFrame
    {
        yyjson_val* pair = nullptr;

        /** [key, value] 배열의 위치 (예: "points[0]") */
        String path;

        /** 다음에 꺼낼 것 (0은 key, 1은 value) */
        usize next_slot = 0;
    };

    /**
     * 읽는 중인 컨테이너 하나
     * JsonWriter::State::OpenContainer와 같은 이유로 구조체로 감쌉니다.
     */
    struct OpenContainer
    {
        std::variant<StructFrame, SeqFrame, ObjectMapFrame, PairMapFrame, ObjectEntryFrame, PairEntryFrame> frame;
    };

    explicit State(Archive& in_archive)
        : archive(in_archive)
    {
    }

    ~State()
    {
        yyjson_doc_free(doc);
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;

    /**
     * 다음 값 하나를 꺼냅니다. 루트는 문서의 루트 값, struct면 Field가 찾아 둔 값, 시퀀스면 다음 원소, 맵 엔트리면 key 다음에 value입니다.
     * 꺼낼 값이 없으면 SetError를 호출하고 nullptr를 돌려줍니다.
     */
    [[nodiscard]] yyjson_val* TakeValue()
    {
        if (archive.HasError())
        {
            return nullptr;
        }

        const auto top = open_containers.Peek();
        if (!top)
        {
            if (is_root_taken)
            {
                archive.SetError("JsonReader: the root value was already read.");
                return nullptr;
            }
            is_root_taken = true;
            return yyjson_doc_get_root(doc);
        }

        const auto value_inside_map = [this] -> yyjson_val*
        {
            archive.SetError("JsonReader: a value inside a map needs BeginMapEntry first.");
            return nullptr;
        };
        const auto past_entry_end = [this] -> yyjson_val*
        {
            archive.SetError("JsonReader: a map entry has only a key and a value.");
            return nullptr;
        };

        return std::visit(Overloaded{
            // Field가 찾아 둔 값
            [this](StructFrame& frame) -> yyjson_val*
            {
                if (frame.field_value == nullptr)
                {
                    archive.SetError("JsonReader: a value inside a struct needs a Field name first.");
                    return nullptr;
                }
                return std::exchange(frame.field_value, nullptr);
            },

            // 다음 원소
            [this](SeqFrame& frame) -> yyjson_val*
            {
                yyjson_val* const element = yyjson_arr_iter_next(&frame.next_element);
                if (element == nullptr)
                {
                    archive.SetError(String::Format("JsonReader: read past the end of an array of length {}.", yyjson_arr_size(frame.array)));
                }
                return element;
            },

            // 맵에서는 BeginMapEntry로 엔트리를 연 뒤에 꺼냄
            [&](const ObjectMapFrame&) -> yyjson_val* { return value_inside_map(); },
            [&](const PairMapFrame&) -> yyjson_val* { return value_inside_map(); },

            // key 다음에 value
            [&](ObjectEntryFrame& frame) -> yyjson_val*
            {
                if (frame.next_slot >= 2)
                {
                    return past_entry_end();
                }
                return frame.next_slot++ == 0 ? frame.key : frame.value;
            },
            [&](PairEntryFrame& frame) -> yyjson_val*
            {
                if (frame.next_slot >= 2)
                {
                    return past_entry_end();
                }
                return yyjson_arr_get(frame.pair, frame.next_slot++);
            },
        }, top->frame);
    }

    /**
     * node를 width와 부호에 맞는 정수로 읽습니다. JSON 숫자와 10진 문자열을 모두 받고, i64를 넘는 u64는 비트를 보존한 i64로 돌려줍니다.
     * 종류가 다르거나 범위를 벗어나면 SetError를 호출하고 NullOpt를 돌려줍니다.
     */
    [[nodiscard]] Optional<i64> ReadInteger(yyjson_val* node, EIntWidth width, bool is_signed)
    {
        if (yyjson_is_int(node))
        {
            i64 number = 0;
            if (yyjson_is_uint(node))
            {
                // 양수는 uint로 저장되므로 i64를 넘는 값만 u64로 받음
                const u64 unsigned_number = yyjson_get_uint(node);
                if (unsigned_number > static_cast<u64>(std::numeric_limits<i64>::max()))
                {
                    if (width != EIntWidth::Bits64 || is_signed)
                    {
                        archive.SetError(String::Format("JsonReader: {} is out of range for {}.", unsigned_number, text_archive::IntTypeName(width, is_signed)));
                        return NullOpt;
                    }
                    return static_cast<i64>(unsigned_number);
                }
                number = static_cast<i64>(unsigned_number);
            }
            else
            {
                number = yyjson_get_sint(node);
            }

            if (!text_archive::FitsInWidth(number, width, is_signed))
            {
                archive.SetError(String::Format("JsonReader: {} is out of range for {}.", number, text_archive::IntTypeName(width, is_signed)));
                return NullOpt;
            }
            return number;
        }

        // 절댓값이 2^53 - 1을 넘는 정수는 10진 문자열로 저장됨
        if (yyjson_is_str(node))
        {
            const std::string_view digits{ yyjson_get_str(node), yyjson_get_len(node) };
            const char* const first = digits.data();
            const char* const last = first + digits.size();
            const bool is_u64 = width == EIntWidth::Bits64 && !is_signed;

            u64 unsigned_number = 0;
            i64 signed_number = 0;
            const auto result = is_u64 ? std::from_chars(first, last, unsigned_number) : std::from_chars(first, last, signed_number);
            if (result.ec == std::errc::result_out_of_range)
            {
                archive.SetError(String::Format("JsonReader: {} is out of range for {}.", digits, text_archive::IntTypeName(width, is_signed)));
                return NullOpt;
            }
            if (result.ec != std::errc{} || result.ptr != last)
            {
                archive.SetError(String::Format("JsonReader: '{}' is not a valid {} number.", digits, text_archive::IntTypeName(width, is_signed)));
                return NullOpt;
            }

            if (is_u64)
            {
                return static_cast<i64>(unsigned_number);
            }
            if (!text_archive::FitsInWidth(signed_number, width, is_signed))
            {
                archive.SetError(String::Format("JsonReader: {} is out of range for {}.", signed_number, text_archive::IntTypeName(width, is_signed)));
                return NullOpt;
            }
            return signed_number;
        }

        archive.SetError(String::Format("JsonReader: expected an integer, got {}.", JsonKindName(node)));
        return NullOpt;
    }

    /**
     * TakeValue로 방금 꺼낸 값의 JSON 안 위치를 만듭니다.
     * struct면 "부모.키", 시퀀스면 "부모[번호]", 객체 맵의 엔트리면 "맵.키", 쌍 배열 맵의 엔트리면 "맵[번호][0 또는 1]"입니다. 루트는 빈 문자열입니다.
     * 예: "window", "items[1]", "scores.alice", "points[0][1]"
     */
    [[nodiscard]] String PathOfTakenValue() const
    {
        const auto top = open_containers.Peek();
        if (!top)
        {
            return {};
        }

        return std::visit(Overloaded{
            // 마지막으로 물어본 필드 이름이 방금 꺼낸 값의 키
            [](const StructFrame& parent) -> String { return text_archive::JoinPath(parent.path, *parent.known_keys.Back()); },

            // TakeValue가 이미 다음 번호로 넘어갔으므로 하나 앞이 방금 꺼낸 원소
            [](const SeqFrame& parent) -> String { return String::Format("{}[{}]", parent.path, parent.next_element.idx - 1); },

            // 객체 맵은 key와 value가 엔트리의 위치 ("scores.alice")
            [](const ObjectEntryFrame& parent) -> String { return parent.path; },

            // 쌍 배열 맵은 [key, value] 배열 안의 번호 ("points[0][1]")
            [](const PairEntryFrame& parent) -> String { return String::Format("{}[{}]", parent.path, parent.next_slot - 1); },

            // TakeValue가 맵에서는 값을 꺼내지 않음
            [](const ObjectMapFrame&) -> String { SE_UNREACHABLE(); },
            [](const PairMapFrame&) -> String { SE_UNREACHABLE(); },
        }, top->frame);
    }

    /** 맨 위 컨테이너가 Frame이면 그 프레임을, 비었거나 다른 종류면 NullOpt를 돌려줍니다. */
    template <typename Frame>
    [[nodiscard]] Optional<Frame&> TopAs()
    {
        const auto top = open_containers.Peek();
        if (!top)
        {
            return NullOpt;
        }
        if (Frame* const frame = std::get_if<Frame>(&top->frame))
        {
            return *frame;
        }
        return NullOpt;
    }

    Archive& archive;

    /** 파싱한 문서. 파싱에 실패했으면 nullptr입니다. */
    yyjson_doc* doc = nullptr;

    Stack<OpenContainer> open_containers;

    /** 객체에 있는데 타입에 없는 키의 경고 */
    Array<String> warnings;

    /** 루트 값을 이미 꺼냈는지 여부 */
    bool is_root_taken = false;
};

JsonReader::JsonReader(StringView text)
    : state(std::make_unique<State>(*this))
{
    yyjson_read_err error{};
    state->doc = yyjson_read_opts(const_cast<char*>(text.Data()), text.ByteLen(), YYJSON_READ_NOFLAG, nullptr, &error);
    if (state->doc == nullptr)
    {
        usize line = 1;
        usize column = 1;
        usize character = 0;
        (void)yyjson_locate_pos(text.Data(), text.ByteLen(), error.pos, &line, &column, &character);
        SetError(String::Format("JsonReader: failed to parse JSON at line {}, column {}: {}", line, column, error.msg));
        return;
    }

    // 중복 키는 구간으로 건너뛰는 객체 안에도 있을 수 있으므로, 읽기 전에 문서 전체를 한 번에 검사
    String path;
    if (const auto duplicate = FindDuplicateKey(yyjson_doc_get_root(state->doc), path, 0))
    {
        SetError(*duplicate);
    }
}

JsonReader::~JsonReader() = default;

ArrayView<const String> JsonReader::GetWarnings() const
{
    return state->warnings;
}

bool JsonReader::IsTextFormat() const
{
    return true;
}

bool JsonReader::SupportsRawElements() const
{
    return false;
}

void JsonReader::Int(i64& value, EIntWidth width, bool is_signed)
{
    yyjson_val* const node = state->TakeValue();
    if (node == nullptr)
    {
        return;
    }

    if (const auto number = state->ReadInteger(node, width, is_signed))
    {
        value = *number;
    }
}

void JsonReader::Float(f64& value, EFloatWidth width)
{
    yyjson_val* const node = state->TakeValue();
    if (node == nullptr)
    {
        return;
    }

    // 사람이 17처럼 정수로 적은 실수도 받음
    if (!yyjson_is_num(node))
    {
        SetError(String::Format("JsonReader: expected a float, got {}.", JsonKindName(node)));
        return;
    }
    const f64 number = yyjson_get_num(node);

    if (width == EFloatWidth::Bits32)
    {
        // f32로 반올림해 무한대가 되는 값만 오류 (FLT_MAX보다 조금 큰 3.4028235e+38은 반올림하면 FLT_MAX)
        const f32 narrowed = static_cast<f32>(number);
        if (std::isinf(narrowed) && std::isfinite(number))
        {
            SetError(String::Format("JsonReader: {} is out of range for f32.", number));
            return;
        }
        value = narrowed;
        return;
    }
    value = number;
}

void JsonReader::Bool(bool& value)
{
    yyjson_val* const node = state->TakeValue();
    if (node == nullptr)
    {
        return;
    }

    if (!yyjson_is_bool(node))
    {
        SetError(String::Format("JsonReader: expected a boolean, got {}.", JsonKindName(node)));
        return;
    }
    value = yyjson_get_bool(node);
}

void JsonReader::Str(String& value)
{
    yyjson_val* const node = state->TakeValue();
    if (node == nullptr)
    {
        return;
    }

    if (!yyjson_is_str(node))
    {
        SetError(String::Format("JsonReader: expected a string, got {}.", JsonKindName(node)));
        return;
    }
    value = String(yyjson_get_str(node), yyjson_get_len(node));
}

void JsonReader::Bytes(void* data, u64 size)
{
    yyjson_val* const node = state->TakeValue();
    if (node == nullptr)
    {
        return;
    }

    if (!yyjson_is_str(node))
    {
        SetError(String::Format("JsonReader: expected a base64 string, got {}.", JsonKindName(node)));
        return;
    }

    const auto bytes = base64::Decode(StringView{ std::string_view{ yyjson_get_str(node), yyjson_get_len(node) } });
    if (!bytes)
    {
        SetError("JsonReader: invalid base64 string.");
        return;
    }
    if (bytes->Len() != size)
    {
        SetError(String::Format("JsonReader: expected {} bytes, got {} bytes of base64 data.", size, bytes->Len()));
        return;
    }
    std::ranges::copy(*bytes, static_cast<u8*>(data));
}

void JsonReader::RawElements([[maybe_unused]] void* data, [[maybe_unused]] u64 size)
{
    SetError("JsonReader: raw element bytes are not supported in a text format.");
}

void JsonReader::Enum(i64& value, EIntWidth width, bool is_signed, ArrayView<const EnumEntry> entries)
{
    yyjson_val* const node = state->TakeValue();
    if (node == nullptr)
    {
        return;
    }

    // 이름으로 적힌 값
    if (yyjson_is_str(node))
    {
        const StringView name = std::string_view{ yyjson_get_str(node), yyjson_get_len(node) };
        const auto* const entry = std::ranges::find(entries, name, &EnumEntry::name);
        if (entry == entries.end())
        {
            SetError(String::Format("JsonReader: '{}' is not a name of this enum.", name));
            return;
        }
        value = entry->value;
        return;
    }

    // 정수로 적힌 값 (이름이 없는 값)
    if (const auto number = state->ReadInteger(node, width, is_signed))
    {
        value = *number;
    }
}

void JsonReader::BeginStruct()
{
    yyjson_val* const node = state->TakeValue();
    if (node == nullptr)
    {
        return;
    }

    if (!yyjson_is_obj(node))
    {
        SetError(String::Format("JsonReader: expected an object, got {}.", JsonKindName(node)));
        return;
    }
    state->open_containers.Push({ State::StructFrame{ .object = node, .path = state->PathOfTakenValue() } });
}

bool JsonReader::Field(StringView name)
{
    if (HasError())
    {
        return false;
    }

    const auto frame = state->TopAs<State::StructFrame>();
    if (!frame)
    {
        SetError(String::Format("JsonReader: field '{}' is outside a struct.", name));
        return false;
    }

    // 타입이 물어본 필드 이름을 기억해 두고, EndStruct에서 타입에 없는 키를 찾음
    frame->known_keys.Push(name);

    // 없는 필드는 false (호출자가 현재 값을 유지)
    frame->field_value = yyjson_obj_getn(frame->object, name.Data(), name.ByteLen());
    return frame->field_value != nullptr;
}

void JsonReader::EndStruct()
{
    if (HasError())
    {
        return;
    }

    const auto frame = state->TopAs<State::StructFrame>();
    if (!frame)
    {
        SetError("JsonReader: EndStruct does not match an open struct.");
        return;
    }

    // 타입에 없는 키는 경고로 남기고 읽기는 계속함 (필드 이름을 바꿨거나 오타를 냈을 때 알아차리게)
    yyjson_obj_iter iter;
    yyjson_obj_iter_init(frame->object, &iter);
    while (yyjson_val* const key = yyjson_obj_iter_next(&iter))
    {
        const StringView name = std::string_view{ yyjson_get_str(key), yyjson_get_len(key) };
        const bool is_known = std::ranges::any_of(frame->known_keys, [&](const String& known_key)
        {
            return known_key == name;
        });
        if (!is_known)
        {
            state->warnings.Push(String::Format("JsonReader: unknown key '{}' is ignored.", text_archive::JoinPath(frame->path, name)));
        }
    }
    state->open_containers.Pop();
}

void JsonReader::BeginSeq(u64& count)
{
    yyjson_val* const node = state->TakeValue();
    if (node == nullptr)
    {
        return;
    }

    if (!yyjson_is_arr(node))
    {
        SetError(String::Format("JsonReader: expected an array, got {}.", JsonKindName(node)));
        return;
    }
    count = yyjson_arr_size(node);

    State::SeqFrame frame{ .array = node, .path = state->PathOfTakenValue() };
    yyjson_arr_iter_init(node, &frame.next_element);
    state->open_containers.Push({ std::move(frame) });
}

void JsonReader::EndSeq()
{
    if (HasError())
    {
        return;
    }

    if (!state->TopAs<State::SeqFrame>())
    {
        SetError("JsonReader: EndSeq does not match an open sequence.");
        return;
    }
    state->open_containers.Pop();
}

void JsonReader::BeginMap(u64& count)
{
    yyjson_val* const node = state->TakeValue();
    if (node == nullptr)
    {
        return;
    }

    // key가 모두 문자열이면 객체, 아니면 [key, value] 쌍 배열로 쓰여 있음 (빈 맵은 둘 다 받음)
    if (yyjson_is_obj(node))
    {
        count = yyjson_obj_size(node);
        state->open_containers.Push({ State::ObjectMapFrame{ .object = node, .path = state->PathOfTakenValue(), .next_entry = yyjson_obj_iter_with(node) } });
        return;
    }
    if (yyjson_is_arr(node))
    {
        count = yyjson_arr_size(node);
        state->open_containers.Push({ State::PairMapFrame{ .pairs = node, .path = state->PathOfTakenValue(), .next_pair = yyjson_arr_iter_with(node) } });
        return;
    }
    SetError(String::Format("JsonReader: expected an object or an array, got {}.", JsonKindName(node)));
}

void JsonReader::BeginMapEntry()
{
    if (HasError())
    {
        return;
    }

    // 객체 맵의 엔트리는 키 노드를 그대로 key 자리에서 읽어, 문자열이나 enum 이름으로 읽히게 함
    if (const auto frame = state->TopAs<State::ObjectMapFrame>())
    {
        yyjson_val* const key = yyjson_obj_iter_next(&frame->next_entry);
        if (key == nullptr)
        {
            SetError(String::Format("JsonReader: read past the end of an object of {} entries.", yyjson_obj_size(frame->object)));
            return;
        }
        const StringView name = std::string_view{ yyjson_get_str(key), yyjson_get_len(key) };
        state->open_containers.Push({ State::ObjectEntryFrame{
            .key = key,
            .value = yyjson_obj_iter_get_val(key),
            .path = text_archive::JoinPath(frame->path, name),
        }, });
        return;
    }

    // 쌍 배열 맵의 엔트리는 원소 하나가 [key, value] 배열
    const auto frame = state->TopAs<State::PairMapFrame>();
    if (!frame)
    {
        SetError("JsonReader: BeginMapEntry is outside a map.");
        return;
    }

    yyjson_val* const pair = yyjson_arr_iter_next(&frame->next_pair);
    if (pair == nullptr)
    {
        SetError(String::Format("JsonReader: read past the end of an array of length {}.", yyjson_arr_size(frame->pairs)));
        return;
    }
    if (!yyjson_is_arr(pair))
    {
        SetError(String::Format("JsonReader: expected a [key, value] array, got {}.", JsonKindName(pair)));
        return;
    }
    if (yyjson_arr_size(pair) != 2)
    {
        SetError(String::Format("JsonReader: expected a [key, value] array, got an array of length {}.", yyjson_arr_size(pair)));
        return;
    }

    // 방금 꺼낸 쌍의 번호는 idx - 1
    state->open_containers.Push({ State::PairEntryFrame{
        .pair = pair,
        .path = String::Format("{}[{}]", frame->path, frame->next_pair.idx - 1),
    }, });
}

void JsonReader::EndMapEntry()
{
    if (HasError())
    {
        return;
    }

    if (!state->TopAs<State::ObjectEntryFrame>() && !state->TopAs<State::PairEntryFrame>())
    {
        SetError("JsonReader: EndMapEntry does not match an open map entry.");
        return;
    }
    state->open_containers.Pop();
}

void JsonReader::EndMap()
{
    if (HasError())
    {
        return;
    }

    if (!state->TopAs<State::ObjectMapFrame>() && !state->TopAs<State::PairMapFrame>())
    {
        SetError("JsonReader: EndMap does not match an open map.");
        return;
    }
    state->open_containers.Pop();
}

void JsonReader::Present(bool& has_value)
{
    // None은 struct 필드의 키를 생략해서만 쓰므로 읽을 값이 있는 자리는 항상 Some (없는 필드는 Field가 false)
    if (!HasError())
    {
        has_value = true;
    }
}

// 텍스트는 값의 경계가 문서 구조에 드러나므로 구간에 읽을 것이 없음
void JsonReader::BeginSection() {}
void JsonReader::EndSection() {}

void JsonReader::SkipSection()
{
    // 구간에는 값이 하나뿐이므로 그 값을 꺼내 버리면 건너뜀
    (void)state->TakeValue();
}

void JsonReader::Rewind()
{
    state->open_containers.Clear();
    state->warnings.Clear();
    state->is_root_taken = false;
}
} // namespace se
