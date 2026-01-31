#pragma once
#include <compare>
#include <format>
#include <iterator>
#include <ranges>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"


namespace se
{
template <typename Allocator>
class BaseString;

using String = BaseString<core::DefaultAllocator<char>>;


namespace details
{
// --- 코드 포인트 이터레이터 ---
class SE_CORE_API CodePointIterator
{
public:
    // C++20 Iterator Traits
    using iterator_category = std::input_iterator_tag;
    using value_type = char32;
    using difference_type = std::ptrdiff_t;
    using pointer = const value_type*;
    using reference = value_type;

    CodePointIterator()
        : ptr(nullptr)
    {
    }

    explicit CodePointIterator(const char* p)
        : ptr(reinterpret_cast<const uint8*>(p))
    {
    }

    reference operator*() const;
    CodePointIterator& operator++();
    CodePointIterator operator++(int);

    bool operator==(const CodePointIterator& other) const { return ptr == other.ptr; }
    bool operator!=(const CodePointIterator& other) const { return ptr != other.ptr; }

private:
    const uint8* ptr;
};

// --- 코드 포인트 뷰 ---
class CodePointView
{
public:
    CodePointView() = default;

    explicit CodePointView(StringView sv)
        : view(sv)
    {
    }

    [[nodiscard]] CodePointIterator begin() const { return CodePointIterator(view.Data()); }
    [[nodiscard]] CodePointIterator end() const { return CodePointIterator(view.Data() + view.ByteLen()); }

    [[nodiscard]] bool IsEmpty() const { return view.IsEmpty(); }

private:
    StringView view;
};
}  // namespace details

/**
 * UTF-8 인코딩을 네이티브로 지원하는 동적 문자열 클래스
 * 모든 인덱스와 길이는 명시되지 않는 한 바이트(char) 기준입니다.
 * @warning 컴파일러 옵션으로 소스 인코딩이 UTF-8임이 보장되어야 합니다.
 * @note 내부적으로 항상 null terminator를 유지합니다.
 * @tparam Allocator 메모리 할당자 타입
 */
template <typename Allocator>
class BaseString
{
public:
    // STL 호환
    using value_type = char;
    using allocator_type = Allocator;
    using size_type = usize;
    using difference_type = isize;

    // 엔진 내부 일관성을 위한 PascalCase 별칭
    using ValueType = value_type;
    using AllocatorType = allocator_type;
    using SizeType = size_type;
    using DifferenceType = difference_type;

public:
    BaseString() noexcept;
    ~BaseString() = default;

    /**
     * 단일 유니코드 코드 포인트로부터 String 객체를 생성합니다.
     * @param code_point 생성할 문자
     * @param repeat 이 문자를 반복할 횟수
     */
    BaseString(char32 code_point, SizeType repeat = 1);
    BaseString& operator=(char32 code_point);

    /**
     * C-style 문자열 리터럴로부터 String 객체를 생성합니다.
     * @param literal null로 끝나는 C-style UTF-8 문자열
     */
    BaseString(const char* literal);
    BaseString(const char* literal, SizeType length);
    BaseString& operator=(const char* literal);

    /**
     * 컴파일 타임에 크기가 알려진 문자열 리터럴로부터 String 객체를 생성합니다.
     * @tparam N 리터럴의 크기 (null terminator 포함).
     * @param literal 문자열 리터럴.
     */
    template <usize N>
    BaseString(const char (&literal)[N]);

    template <usize N>
    BaseString& operator=(const char (&literal)[N]);

    /**
     * StringView로부터 String 객체를 생성합니다.
     * @param view StringView
     */
    BaseString(StringView view);
    BaseString& operator=(StringView view);

    /**
     * Iterator로부터 문자열을 생성합니다.
     * @param first 시작 이터레이터
     * @param last 끝 이터레이터
     */
    template <std::input_iterator It, std::sentinel_for<It> Sent>
        requires std::same_as<std::iter_value_t<It>, char>
    BaseString(It first, Sent last);

    BaseString(const BaseString&) = default;
    BaseString& operator=(const BaseString&) = default;
    BaseString(BaseString&&) noexcept = default;
    BaseString& operator=(BaseString&&) noexcept = default;

public:
    /**
     * C++20 std::format을 사용하여 포맷된 문자열을 생성합니다.
     * @param fmt 포맷 문자열
     * @param args 포맷 인자
     * @return 포맷팅된 새로운 String 객체
     */
    template <typename... Args>
    [[nodiscard]] static BaseString Format(std::format_string<Args...> fmt, Args&&... args);

    /**
     * C++20 Range로부터 문자열을 생성합니다.
     * @param range char로 변환 가능한 요소를 포함하는 range
     */
    template <std::ranges::input_range Rng>
        requires std::same_as<std::ranges::range_value_t<Rng>, char>
    [[nodiscard]] static BaseString FromRange(Rng&& range);

public:
    /**
     * 문자열의 바이트 길이를 반환합니다. (null terminator 제외)
     * @return 바이트 단위 길이 (O(1))
     */
    [[nodiscard]] SizeType ByteLen() const noexcept;

    /**
     * 유니코드 코드 포인트(논리적 '문자')의 개수를 반환합니다.
     * @return 코드 포인트의 개수 (O(N), N은 바이트 길이)
     * @note ByteLen()보다 비용이 높은 연산입니다.
     */
    [[nodiscard]] SizeType CodePointLen() const;

    /** 문자열이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /**
     * 현재 예약된 메모리 용량을 바이트 단위로 반환합니다.
     * @return 재할당 없이 저장 가능한 바이트 수 (null terminator 공간 제외)
     */
    [[nodiscard]] SizeType Capacity() const noexcept;

    /**
     * 최소 new_capacity 바이트를 저장할 공간을 예약합니다.
     * @param new_capacity 예약할 바이트 수
     */
    void Reserve(SizeType new_capacity);

    /**
     * 문자열의 크기를 new_size로 변경합니다. 추가된 메모리 공간은 초기화되지 않습니다.
     * @param new_size 변경할 문자열의 길이 (Null 문자 제외)
     */
    void ResizeForOverwrite(SizeType new_size);

    /** 메모리 용량을 실제 크기에 맞게 줄입니다. */
    void ShrinkToFit();

public:
    /**
     * 문자열 끝에 단일 코드 포인트를 추가합니다.
     * @param code_point 추가할 유니코드 코드 포인트 (char32_t)
     */
    void Push(char32 code_point);

    /**
     * 문자열의 마지막 코드 포인트를 제거하고 그 값을 반환합니다.
     * @return 제거된 코드 포인트. 문자열이 비어있으면 nullopt
     */
    Optional<char32> Pop();

    /**
     * 문자열 끝에 다른 문자열을 덧붙입니다.
     * @param other 덧붙일 String 객체
     */
    void Append(const BaseString& other);
    void Append(const char* str);
    void Append(StringView view);

    /**
     * 바이트 인덱스 위치에 문자열을 삽입합니다.
     * @param byte_idx 삽입할 위치의 바이트 오프셋
     * @param view 삽입할 문자열 뷰
     * @warning byte_idx는 유효한 UTF-8 코드 포인트 경계여야 합니다.
     */
    void Insert(SizeType byte_idx, StringView view);

    /**
     * 바이트 인덱스 위치부터 count 바이트만큼 문자를 제거합니다.
     * @param byte_idx 제거를 시작할 바이트 오프셋
     * @param count 제거할 바이트 수
     * @warning byte_idx와 byte_idx + count는 유효한 UTF-8 코드 포인트 경계여야 합니다.
     */
    void RemoveRange(SizeType byte_idx, SizeType count);

    /**
     * 문자열을 new_byte_len 바이트로 자릅니다.
     * @param new_byte_len 새로운 바이트 길이
     * @warning new_byte_len은 유효한 UTF-8 코드 포인트 경계여야 합니다.
     */
    void Truncate(SizeType new_byte_len);

    /** 문자열의 모든 내용을 지웁니다. */
    void Clear() noexcept;

    /**
     * 문자열을 대문자로 변환한 새로운 String 객체를 반환합니다.
     * @param locale 로케일 ID (예: "en_US", "tr_TR"). 비워두면 기본 로케일 사용.
     * @note 내부적으로 ICU4C를 사용합니다.
     */
    [[nodiscard]] BaseString ToUpper(const char* locale = "") const;

    /**
     * 문자열을 소문자로 변환한 새로운 String 객체를 반환합니다.
     * @param locale 로케일 ID (예: "en_US", "tr_TR"). 비워두면 기본 로케일 사용.
     * @note 내부적으로 ICU4C를 사용합니다.
     */
    [[nodiscard]] BaseString ToLower(const char* locale = "") const;

    /**
     * 문자열에 특정 부분 문자열이 포함되어 있는지 확인합니다.
     * @param view 검색할 StringView
     * @return 포함되어 있으면 true
     */
    [[nodiscard]] bool Contains(StringView view) const;

    /**
     * 문자열이 특정 접두사로 시작하는지 확인합니다.
     * @param view 비교할 접두사 view
     * @return 접두사로 시작하면 true
     */
    [[nodiscard]] bool StartsWith(StringView view) const;

    /**
     * 문자열이 특정 접미사로 끝나는지 확인합니다.
     * @param view 비교할 접미사 view
     * @return 접미사로 끝나면 true
     */
    [[nodiscard]] bool EndsWith(StringView view) const;

    /**
     * 부분 문자열을 검색하여 첫 번째로 일치하는 위치의 바이트 인덱스를 반환합니다.
     * @param view 검색할 StringView
     * @param start_byte_pos 검색을 시작할 바이트 오프셋
     * @return 찾은 경우 해당 바이트 인덱스, 찾지 못한 경우 nullopt
     */
    [[nodiscard]] Optional<SizeType> Find(StringView view, SizeType start_byte_pos = 0) const;

    /**
     * 부분 문자열을 뒤에서부터 검색하여 첫 번째로 일치하는 위치의 바이트 인덱스를 반환합니다.
     * @param view 검색할 StringView
     * @param start_byte_pos 검색을 시작할 바이트 오프셋 (기본값: 끝에서부터)
     * @return 찾은 경우 해당 바이트 인덱스, 찾지 못한 경우 nullopt
     */
    [[nodiscard]] Optional<SizeType> FindLast(StringView view, SizeType start_byte_pos = -1) const;

    /**
     * 바이트 인덱스 기준으로 부분 문자열을 복사하여 새로운 String 객체를 반환합니다.
     * @param start_index 시작 바이트 인덱스
     * @param byte_count 복사할 바이트 수 (기본값: 끝까지)
     * @return 새로운 String 객체
     */
    [[nodiscard]] BaseString Substring(SizeType start_index, SizeType byte_count = -1) const;

    /**
     * 바이트 인덱스 기준으로 부분 StringView를 반환합니다. (메모리 할당 없음)
     * @param start_index 시작 바이트 인덱스
     * @param byte_count 뷰가 가리킬 바이트 수 (기본값: 끝까지)
     * @return StringView 객체
     */
    [[nodiscard]] StringView SubstringView(SizeType start_index, SizeType byte_count = -1) const;

public:
    /**
     * 내부 데이터 버퍼에 대한 읽기 전용 C-style 포인터를 반환합니다. (null-terminated 보장)
     * @return 읽기 전용 데이터 포인터
     */
    [[nodiscard]] const char* CStr() const noexcept;

    /**
     * 내부 데이터 버퍼에 대한 읽기 전용 포인터를 반환합니다.
     * @return 읽기 전용 데이터 포인터
     */
    [[nodiscard]] const char* Data() const noexcept;

    /**
     * 내부 데이터 버퍼에 대한 쓰기 가능한 포인터를 반환합니다.
     * @return 쓰기 가능한 데이터 포인터
     * @warning 이 포인터를 통해 문자열의 길이를 바꾸거나 중간에 null을 삽입하면 클래스의 불변성이 깨질 수 있습니다.
     */
    [[nodiscard]] char* Data() noexcept;

    /**
     * 코드 포인트(논리적 문자) 단위로 순회할 수 있는 range-like 뷰를 반환합니다.
     * @code
     * for (char32 cp : my_string.CodePoints()) { ... }
     * @endcode
     */
    [[nodiscard]] details::CodePointView CodePoints() const;

    /**
     * 바이트(코드 유닛) 단위로 순회할 수 있는 뷰를 반환합니다.
     * @code
     * for (char byte : my_string.Bytes()) { ... }
     * @endcode
     */
    [[nodiscard]] StringView Bytes() const;

    /** 문자열을 서로 교환합니다. */
    void Swap(BaseString& other) noexcept;

public:
    [[nodiscard]] BaseString operator+(const BaseString& other) const;
    [[nodiscard]] BaseString operator+(char32 code_point) const;
    [[nodiscard]] BaseString operator+(const char* str) const;
    [[nodiscard]] BaseString operator+(StringView view) const;

    [[nodiscard]] friend BaseString operator+(char32 lhs, const BaseString& rhs)
    {
        BaseString ret{ lhs };
        ret.Append(rhs);
        return ret;
    }

    [[nodiscard]] friend BaseString operator+(const char* lhs, const BaseString& rhs)
    {
        BaseString ret{ lhs };
        ret.Append(rhs);
        return ret;
    }

    [[nodiscard]] friend BaseString operator+(StringView lhs, const BaseString& rhs)
    {
        BaseString ret{ lhs };
        ret.Append(rhs);
        return ret;
    }

    BaseString& operator+=(const BaseString& other);
    BaseString& operator+=(char32 code_point);
    BaseString& operator+=(const char* str);
    BaseString& operator+=(StringView view);

    [[nodiscard]] bool operator==(const BaseString& other) const;
    [[nodiscard]] bool operator==(const char* str) const;
    [[nodiscard]] bool operator==(StringView view) const;

    [[nodiscard]] friend bool operator==(const char* lhs, const BaseString& rhs)
    {
        return StringView{ lhs } == StringView{ rhs };
    }

    [[nodiscard]] friend bool operator==(StringView lhs, const BaseString& rhs)
    {
        return lhs == StringView{ rhs };
    }

    [[nodiscard]] std::strong_ordering operator<=>(const BaseString& other) const;
    [[nodiscard]] std::strong_ordering operator<=>(const char* other) const;
    [[nodiscard]] std::strong_ordering operator<=>(StringView other) const;

    [[nodiscard]] friend std::strong_ordering operator<=>(const char* lhs, const BaseString& rhs)
    {
        return StringView{ lhs } <=> StringView{ rhs };
    }

    [[nodiscard]] friend std::strong_ordering operator<=>(StringView lhs, const BaseString& rhs)
    {
        return lhs <=> StringView{ rhs };
    }

    friend void swap(BaseString& lhs, BaseString& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

private:
    // TODO: 나중에 필요할 때 SSO 구현
    Array<char, AllocatorType> data;
};
}  // namespace se

// se::String에 대한 std::hash 특수화
template <>
struct std::hash<se::String>
{
    size_t operator()(const se::String& path) const noexcept
    {
        return std::hash<se::StringView>{}(se::StringView{ path });
    }
};

// se::String에 대한 std::formatter 특수화
template <>
struct std::formatter<se::String, char> : std::formatter<se::StringView>
{
    auto format(const se::String& string, std::format_context& ctx) const
    {
        const se::StringView sv{ string };
        return std::formatter<se::StringView>::format(sv, ctx);
    }
};

#include "SimpleEngine/Core/Container/String.inl"
