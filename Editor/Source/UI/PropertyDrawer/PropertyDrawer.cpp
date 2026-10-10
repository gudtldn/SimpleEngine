#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

#include "UI/ImGui/ImGuiString.h"
#include "UI/ImGui/ImGuiWrapper.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/DisplayAnnotations.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Reflection/ValueOpsRegistry.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/ECS/Entity.h"
#include "SimpleEngine/Traits/TypeTraits.h"

#include "imgui.h"

#include <algorithm>
#include <cstdio>
#include <ranges>


namespace se::editor
{
namespace
{
// ============================================================================
// ImGuiDataType Mapping
// ============================================================================

template <typename T>
consteval ImGuiDataType_ GetImGuiDataType()
{
    if constexpr (std::same_as<T, i8>)        { return ImGuiDataType_S8;     }
    else if constexpr (std::same_as<T, u8>)  { return ImGuiDataType_U8;     }
    else if constexpr (std::same_as<T, i16>)  { return ImGuiDataType_S16;    }
    else if constexpr (std::same_as<T, u16>) { return ImGuiDataType_U16;    }
    else if constexpr (std::same_as<T, i32>)  { return ImGuiDataType_S32;    }
    else if constexpr (std::same_as<T, u32>) { return ImGuiDataType_U32;    }
    else if constexpr (std::same_as<T, i64>)  { return ImGuiDataType_S64;    }
    else if constexpr (std::same_as<T, u64>) { return ImGuiDataType_U64;    }
    else if constexpr (std::same_as<T, f32>)  { return ImGuiDataType_Float;  }
    else if constexpr (std::same_as<T, f64>) { return ImGuiDataType_Double; }
    else
    {
        static_assert(se::traits::AlwaysFalse<T>, "Unsupported type for ImGuiDataType conversion.");
        SE_UNREACHABLE();
    }
}

// ============================================================================
// Built-in Drawers
// ============================================================================

// --- Bool ---

bool DrawBool(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    return ImGui::Checkbox(label, static_cast<bool*>(value));
}

// --- Arithmetic (int, uint, f32, f64) ---

template <typename T>
bool DrawArithmetic(const char* label, void* value, const AnnotationList& annotations)
{
    constexpr ImGuiDataType_ DATA_TYPE = GetImGuiDataType<T>();
    T* v = static_cast<T*>(value);

    if (const auto range = annotations.Find<display::RangeAnnotation>())
    {
        T min_val = static_cast<T>(range->min);
        T max_val = static_cast<T>(range->max);
        return ImGui::SliderScalar(label, DATA_TYPE, v, &min_val, &max_val);
    }

    constexpr f32 SPEED = std::floating_point<T> ? 0.1f : 1.0f;
    return ImGui::DragScalarNInfinity(label, DATA_TYPE, v, 1, SPEED);
}

// --- String ---

bool DrawString(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    String& str = *static_cast<String*>(value);
    return ImGui::InputText(label, &str);
}

// --- StringName (read-only: interned string) ---

bool DrawStringName(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    const StringName& name = *static_cast<StringName*>(value);
    ImGui::LabelText(label, "%s", name.CStr());
    return false;
}

// --- Guid (read-only) ---

bool DrawGuid(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    const Guid& guid = *static_cast<Guid*>(value);
    const String str = guid.ToString();
    ImGui::LabelText(label, "%s", str.CStr());
    return false;
}

// --- TypeId (read-only) ---

bool DrawTypeId(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    const TypeId type_id = *static_cast<TypeId*>(value);
    if (type_id.IsNull())
    {
        ImGui::LabelText(label, "(none)");
    }
    else if (const auto type_info = TypeRegistry::Get().Find(type_id))
    {
        ImGui::LabelText(label, "%.*s", static_cast<int>(type_info->name.ByteLen()), type_info->name.Data());
    }
    else
    {
        ImGui::LabelText(label, "%016llx", static_cast<unsigned long long>(type_id.Value()));
    }
    return false;
}

// --- AssetId (GUID 표시 + Asset Drag&Drop Target) ---

bool DrawAssetId(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    AssetId& asset_id = *static_cast<AssetId*>(value);
    bool modified = false;

    if (asset_id.IsValid())
    {
        const String str = asset_id.GetGuid().ToString();
        ImGui::LabelText(label, "%s", str.CStr());
    }
    else
    {
        // 드롭 대상임을 시각적으로 표시
        ImGui::LabelText(label, "(none \xe2\x80\x94 drop asset here)");
    }

    // Drag&Drop 수신 (AssetsBrowserPanel의 "CONTENT_BROWSER_ITEM" 페이로드)
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
        {
            const char* dropped_path = static_cast<const char*>(payload->Data);

            // DrawerRegistry에 등록된 resolver를 통해 경로 -> AssetId 변환
            if (const AssetDropResolverFunc resolver = DrawerRegistry::Get().GetAssetDropResolver())
            {
                const AssetId resolved = resolver(dropped_path);
                if (resolved.IsValid())
                {
                    asset_id = resolved;
                    modified = true;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    return modified;
}

// --- Entity (read-only) ---

bool DrawEntity(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    const Entity& entity = *static_cast<Entity*>(value);
    if (entity.IsValid())
    {
        ImGui::LabelText(label, "Entity %u (gen %u)", entity.GetId(), entity.GetGeneration());
    }
    else
    {
        ImGui::LabelText(label, "(invalid)");
    }
    return false;
}

// ============================================================================
// Math Drawers
// ============================================================================

// --- Vector2 / Vector2f ---

template <typename T>
bool DrawVector2(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    using Vec = math::Vector2Impl<T>;
    constexpr ImGuiDataType_ DATA_TYPE = GetImGuiDataType<T>();
    Vec* vec = static_cast<Vec*>(value);

    return ImGui::DragScalarNInfinity(label, DATA_TYPE, &vec->x, 2, 0.1f);
}

// --- Vector3 / Vector3f ---

template <typename T>
bool DrawVector3(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    using Vec = math::Vector3Impl<T>;
    constexpr ImGuiDataType_ DATA_TYPE = GetImGuiDataType<T>();
    Vec* vec = static_cast<Vec*>(value);

    return ImGui::DragScalarNInfinity(label, DATA_TYPE, &vec->x, 3, 0.1f);
}

// --- Vector4 / Vector4f ---

template <typename T>
bool DrawVector4(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    using Vec = math::Vector4Impl<T>;
    constexpr ImGuiDataType_ DATA_TYPE = GetImGuiDataType<T>();
    Vec* vec = static_cast<Vec*>(value);

    return ImGui::DragScalarNInfinity(label, DATA_TYPE, &vec->x, 4, 0.1f);
}

// --- Quaternion / Quaternionf ---

template <typename T>
bool DrawQuaternion(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    using Quat = math::QuaternionImpl<T>;
    constexpr ImGuiDataType_ DATA_TYPE = GetImGuiDataType<T>();
    Quat* quat = static_cast<Quat*>(value);
    return ImGui::DragScalarNInfinity(label, DATA_TYPE, &quat->x, 4, 0.01f);
}

// --- Rotator / Rotatorf ---

template <typename T>
bool DrawRotator(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    using Rot = math::RotatorImpl<T>;
    constexpr ImGuiDataType_ DATA_TYPE = GetImGuiDataType<T>();
    Rot* rot = static_cast<Rot*>(value);

    // pitch, roll, yaw — 각각 Degree<T>이므로 .value 멤버에 직접 접근
    return ImGui::DragScalarNInfinity(label, DATA_TYPE, &rot->pitch.value, 3, 0.1f);
}

// --- Matrix4x4 / Matrix4x4f ---
template <typename T>
bool DrawMatrix4x4(const char* label, void* value, const AnnotationList& annotations)
{
    using Mat = math::Matrix4x4Impl<T>;
    constexpr ImGuiDataType_ DATA_TYPE = GetImGuiDataType<T>();
    Mat* mat = static_cast<Mat*>(value);

    bool modified = false;

    const bool read_only = annotations.Has<display::ReadOnlyAnnotation>();

    if (read_only)
    {
        ImGui::EndDisabled();
    }

    if (ImGui::TreeNodeEx(label))
    {
        if (read_only)
        {
            ImGui::BeginDisabled();
        }

        for (int i = 0; i < 4; ++i)
        {
            ImGui::PushID(i);

            // 각 행의 시작 메모리 주소 계산 (row-major 기준)
            T* row_ptr = &mat->data[i * 4];

            // 라벨 설정
            char row_label[16];
            std::snprintf(row_label, sizeof(row_label), "[%d]", i);

            modified |= ImGui::DragScalarNInfinity(row_label, DATA_TYPE, row_ptr, 4, 0.01f);

            ImGui::PopID();
        }

        if (read_only)
        {
            ImGui::EndDisabled();
        }

        ImGui::TreePop();
    }

    if (read_only)
    {
        ImGui::BeginDisabled();
    }

    return modified;
}

// --- LinearColor ---

bool DrawLinearColor(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    LinearColor* color = static_cast<LinearColor*>(value);
    return ImGui::ColorEdit4(label, &color->r);
}

// --- Color (u8 RGBA) ---

bool DrawColor(const char* label, void* value, const AnnotationList& /*annotations*/)
{
    Color* color = static_cast<Color*>(value);
    f32 rgba[4] = {
        static_cast<f32>(color->r) / 255.0f,
        static_cast<f32>(color->g) / 255.0f,
        static_cast<f32>(color->b) / 255.0f,
        static_cast<f32>(color->a) / 255.0f,
    };

    if (ImGui::ColorEdit4(label, rgba))
    {
        color->r = math::RoundToInt<u8>(rgba[0] * 255.0f);
        color->g = math::RoundToInt<u8>(rgba[1] * 255.0f);
        color->b = math::RoundToInt<u8>(rgba[2] * 255.0f);
        color->a = math::RoundToInt<u8>(rgba[3] * 255.0f);
        return true;
    }
    return false;
}

// --- Degree<T> (AngleType) ---

template <typename T>
bool DrawDegree(const char* label, void* value, const AnnotationList& annotations)
{
    using Deg = Degree<T>;
    constexpr ImGuiDataType_ DATA_TYPE = GetImGuiDataType<T>();
    Deg* angle = static_cast<Deg*>(value);

    if (const auto range = annotations.Find<display::RangeAnnotation>())
    {
        T min_val = static_cast<T>(range->min);
        T max_val = static_cast<T>(range->max);
        return ImGui::SliderScalar(label, DATA_TYPE, &angle->value, &min_val, &max_val);
    }

    return ImGui::DragScalarNInfinity(label, DATA_TYPE, &angle->value, 1, 0.1f);
}

// ============================================================================
// Enum Helpers
// ============================================================================

/** type-erased enum 값을 i64로 읽기 (signed/unsigned 대응) */
i64 ReadEnumValue(const void* value, usize size, bool is_unsigned)
{
    if (is_unsigned)
    {
        switch (size)
        {
        case sizeof(u8):
            return static_cast<i64>(*static_cast<const u8*>(value));
        case sizeof(u16):
            return static_cast<i64>(*static_cast<const u16*>(value));
        case sizeof(u32):
            return static_cast<i64>(*static_cast<const u32*>(value));
        case sizeof(u64):
            return static_cast<i64>(*static_cast<const u64*>(value));
        default:
            break;
        }
    }
    else
    {
        switch (size)
        {
        case sizeof(i8):
            return static_cast<i64>(*static_cast<const i8*>(value));
        case sizeof(i16):
            return static_cast<i64>(*static_cast<const i16*>(value));
        case sizeof(i32):
            return static_cast<i64>(*static_cast<const i32*>(value));
        case sizeof(i64):
            return *static_cast<const i64*>(value);
        default:
            break;
        }
    }
    return 0;
}

/** type-erased enum에 i64 값 쓰기 (signed/unsigned 대응) */
void WriteEnumValue(void* value, i64 new_value, usize size, bool is_unsigned)
{
    if (is_unsigned)
    {
        switch (size)
        {
        case sizeof(u8):
            *static_cast<u8*>(value) = static_cast<u8>(new_value);
            break;
        case sizeof(u16):
            *static_cast<u16*>(value) = static_cast<u16>(new_value);
            break;
        case sizeof(u32):
            *static_cast<u32*>(value) = static_cast<u32>(new_value);
            break;
        case sizeof(u64):
            *static_cast<u64*>(value) = static_cast<u64>(new_value);
            break;
        default: break;
        }
    }
    else
    {
        switch (size)
        {
        case sizeof(i8):
            *static_cast<i8*>(value) = static_cast<i8>(new_value);
            break;
        case sizeof(i16):
            *static_cast<i16*>(value) = static_cast<i16>(new_value);
            break;
        case sizeof(i32):
            *static_cast<i32*>(value) = static_cast<i32>(new_value);
            break;
        case sizeof(i64):
            *static_cast<i64*>(value) = new_value;
            break;
        default:
            break;
        }
    }
}

/** enum의 기반 타입이 부호 없는 정수인지 확인합니다. */
bool IsUnsignedUnderlying(TypeId underlying)
{
    return underlying == TypeId::Of<u8>()
        || underlying == TypeId::Of<u16>()
        || underlying == TypeId::Of<u32>()
        || underlying == TypeId::Of<u64>();
}

// ============================================================================
// Enum Drawer (generic, type-erased)
// ============================================================================

bool DrawEnum(const char* label, void* value, const TypeInfo& type_info, const EnumInfo& enum_info)
{
    const bool is_unsigned = IsUnsignedUnderlying(enum_info.underlying);
    const i64 current_value = ReadEnumValue(value, type_info.size, is_unsigned);

    // 이름 없이 등록된 enum은 값만 표시
    if (enum_info.entries.IsEmpty())
    {
        ImGui::LabelText(label, "%lld", static_cast<long long>(current_value));
        return false;
    }

    const auto current = std::ranges::find(enum_info.entries, current_value, &EnumEntry::value);

    // TODO: 나중에 최적화 하려면 LinearAllocator로 최적화
    const String preview = (current != enum_info.entries.end()) ? String{ current->name } : String{ "???" };

    bool modified = false;
    if (ImGui::BeginCombo(label, preview.CStr()))
    {
        for (const EnumEntry& entry : enum_info.entries)
        {
            const String entry_name = entry.name;
            const bool is_selected = (entry.value == current_value);
            if (ImGui::Selectable(entry_name.CStr(), is_selected))
            {
                WriteEnumValue(value, entry.value, type_info.size, is_unsigned);
                modified = true;
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    return modified;
}

// ============================================================================
// BitFlag Enum Drawer (checkbox per flag)
// ============================================================================

bool DrawBitFlags(const char* label, void* value, const TypeInfo& type_info, const EnumInfo& enum_info, bool read_only)
{
    if (enum_info.entries.IsEmpty())
    {
        ImGui::LabelText(label, "[Empty bitflag]");
        return false;
    }

    const bool is_unsigned = IsUnsignedUnderlying(enum_info.underlying);
    i64 current_value = ReadEnumValue(value, type_info.size, is_unsigned);

    bool modified = false;
    if (ImGui::TreeNode(label))
    {
        ImGui::BeginDisabled(read_only);
        for (const EnumEntry& entry : enum_info.entries)
        {
            const i64 flag = entry.value;
            bool has_flag = (current_value & flag) == flag;

            const String entry_name = entry.name;
            if (ImGui::Checkbox(entry_name.CStr(), &has_flag))
            {
                if (has_flag)
                {
                    current_value |= flag;
                }
                else
                {
                    current_value &= ~flag;
                }
                modified = true;
            }
        }
        ImGui::EndDisabled();

        if (modified)
        {
            WriteEnumValue(value, current_value, type_info.size, is_unsigned);
        }

        ImGui::TreePop();
    }
    return modified;
}

// ============================================================================
// Container Drawer Helpers
// ============================================================================

/** 타입의 ValueOps에서 Ops 형태의 연산을 찾습니다. 없으면 NullOpt입니다. */
template <typename Ops>
Optional<const Ops&> FindShapeOps(TypeId type_id)
{
    const auto value_ops = ValueOpsRegistry::Get().Find(type_id);
    if (!value_ops)
    {
        return NullOpt;
    }
    return VariantGet<Ops>(value_ops->shape_ops);
}

/** 컨테이너에 옮겨 넣을 기본 생성 임시 값입니다. 기본 생성할 수 없는 타입이면 Get()이 nullptr입니다. */
class DefaultTemp
{
public:
    explicit DefaultTemp(TypeId type_id)
    {
        const auto value_ops = ValueOpsRegistry::Get().Find(type_id);
        if (value_ops && value_ops->new_object && value_ops->delete_object)
        {
            deleter = value_ops->delete_object;
            object = value_ops->new_object();
        }
    }

    ~DefaultTemp()
    {
        if (object)
        {
            deleter(object);
        }
    }

    DefaultTemp(const DefaultTemp&) = delete;
    DefaultTemp& operator=(const DefaultTemp&) = delete;
    DefaultTemp(DefaultTemp&&) = delete;
    DefaultTemp& operator=(DefaultTemp&&) = delete;

    [[nodiscard]] void* Get() const { return object; }

private:
    void* object = nullptr;
    void (*deleter)(void*) = nullptr;
};

/** "label (N)" 헤더의 TreeNode를 엽니다. */
bool BeginContainerNode(const char* label, usize count)
{
    char header[256];
    std::snprintf(header, sizeof(header), "%s (%zu)", label, count);
    return ImGui::TreeNode(header);
}

bool DrawArrayProperty(
    const char* label,
    void* container,
    const ArrayInfo& info,
    const ArrayOps& ops,
    DrawerRegistry& registry,
    bool read_only
)
{
    const usize count = ops.len(container);
    if (!BeginContainerNode(label, count))
    {
        return false;
    }

    // [+] [-] [Clear] 버튼 (요소 단위 삭제 연산이 없어 끝에서만 줄임)
    bool resized = false;
    if (!read_only && ops.resize)
    {
        if (ImGui::SmallButton("+"))
        {
            ops.resize(container, count + 1);
            resized = true;
        }

        if (count > 0)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("-"))
            {
                ops.resize(container, count - 1);
                resized = true;
            }

            ImGui::SameLine();
            if (ImGui::SmallButton("Clear"))
            {
                ops.resize(container, 0);
                resized = true;
            }
        }
    }

    // 크기가 바뀐 프레임은 요소를 그리지 않음
    bool modified = resized;
    if (!resized)
    {
        for (usize idx = 0; idx < count; ++idx)
        {
            ImGui::PushID(static_cast<int>(idx));

            char elem_label[32];
            std::snprintf(elem_label, sizeof(elem_label), "[%zu]", idx);
            modified |= registry.DrawValue(info.element, elem_label, ops.element_at_mut(container, idx), {}, read_only);

            ImGui::PopID();
        }
    }

    ImGui::TreePop();
    return modified;
}

bool DrawSetProperty(
    const char* label,
    void* container,
    const SetInfo& info,
    const SetOps& ops,
    DrawerRegistry& registry,
    bool read_only
)
{
    const usize count = ops.len(container);
    if (!BeginContainerNode(label, count))
    {
        return false;
    }

    // [+] [Clear] 버튼
    bool changed = false;
    if (!read_only)
    {
        if (ops.emplace_moved && ImGui::SmallButton("+"))
        {
            const DefaultTemp element{ info.element };
            if (element.Get())
            {
                ops.emplace_moved(container, element.Get());
                changed = true;
            }
        }

        if (count > 0)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear"))
            {
                ops.clear(container);
                changed = true;
            }
        }
    }

    if (!changed)
    {
        struct VisitState
        {
            DrawerRegistry* registry;
            TypeId element;
            usize index;
        };
        VisitState state{ .registry = &registry, .element = info.element, .index = 0 };

        ops.for_each(container, [](const void* element, void* user_data)
        {
            VisitState& s = *static_cast<VisitState*>(user_data);
            ImGui::PushID(static_cast<int>(s.index));

            char elem_label[32];
            std::snprintf(elem_label, sizeof(elem_label), "[%zu]", s.index);

            // Set 요소는 해시 불변식 때문에 읽기 전용으로만 그리므로 값이 바뀌지 않음
            s.registry->DrawValue(s.element, elem_label, const_cast<void*>(element), {}, true);

            ImGui::PopID();
            ++s.index;
        }, &state);
    }

    ImGui::TreePop();
    return changed;
}

bool DrawMapProperty(
    const char* label,
    void* container,
    const MapInfo& info,
    const MapOps& ops,
    DrawerRegistry& registry,
    bool read_only
)
{
    const usize count = ops.len(container);
    if (!BeginContainerNode(label, count))
    {
        return false;
    }

    // [+] [Clear] 버튼
    bool changed = false;
    if (!read_only)
    {
        if (ops.emplace_moved && ImGui::SmallButton("+"))
        {
            const DefaultTemp key{ info.key };
            const DefaultTemp value{ info.value };
            if (key.Get() && value.Get())
            {
                ops.emplace_moved(container, key.Get(), value.Get());
                changed = true;
            }
        }

        if (count > 0)
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("Clear"))
            {
                ops.clear(container);
                changed = true;
            }
        }
    }

    bool modified = changed;
    if (!changed)
    {
        struct VisitState
        {
            DrawerRegistry* registry;
            const MapInfo* info;
            bool read_only;
            bool modified;
            usize index;
        };
        VisitState state{ .registry = &registry, .info = &info, .read_only = read_only, .modified = false, .index = 0 };

        ops.for_each_mut(container, [](const void* key, void* value, void* user_data)
        {
            VisitState& s = *static_cast<VisitState*>(user_data);
            ImGui::PushID(static_cast<int>(s.index));

            char entry_label[32];
            std::snprintf(entry_label, sizeof(entry_label), "[%zu]", s.index);

            if (ImGui::TreeNode(entry_label))
            {
                // Key는 해시 불변식 때문에 읽기 전용으로만 그리므로 값이 바뀌지 않음
                s.registry->DrawValue(s.info->key, "Key", const_cast<void*>(key), {}, true);
                s.modified |= s.registry->DrawValue(s.info->value, "Value", value, {}, s.read_only);
                ImGui::TreePop();
            }

            ImGui::PopID();
            ++s.index;
        }, &state);
        modified = state.modified;
    }

    ImGui::TreePop();
    return modified;
}

bool DrawOptionalProperty(
    const char* label,
    void* optional,
    const OptionalInfo& info,
    const OptionalOps& ops,
    DrawerRegistry& registry,
    bool read_only
)
{
    bool modified = false;
    bool has_value = ops.has_value(optional);

    ImGui::PushID(label);

    // [ ] Label
    ImGui::BeginDisabled(read_only || (!has_value && !ops.emplace));
    if (ImGui::Checkbox("##has_value", &has_value))
    {
        if (has_value)
        {
            ops.emplace(optional);
        }
        else
        {
            ops.reset(optional);
        }
        modified = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();

    if (ops.has_value(optional))
    {
        // 값 편집 위젯 (한 줄에 표시)
        const f32 available_width = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(available_width - ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);

        modified |= registry.DrawValue(info.inner, label, ops.value_mut(optional), {}, read_only);

        // 오버라이드 해제(Reset) 버튼
        ImGui::SameLine();
        if (!read_only && ImGui::Button("x", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight())))
        {
            ops.reset(optional);
            modified = true;
        }
    }
    else
    {
        ImGui::BeginDisabled();
        ImGui::Text("%s (Default)", label);
        ImGui::EndDisabled();
    }

    ImGui::PopID();
    return modified;
}

/** 그릴 수 없는 값은 타입 이름만 표시합니다. */
void DrawTypeName(const char* label, const TypeInfo& type_info)
{
    ImGui::LabelText(label, "[%.*s]", static_cast<int>(type_info.name.ByteLen()), type_info.name.Data());
}
} // namespace


// ============================================================================
// DrawerRegistry
// ============================================================================

DrawerRegistry::DrawerRegistry()
{
    RegisterBuiltinDrawers();
}

DrawerRegistry& DrawerRegistry::Get()
{
    static DrawerRegistry instance;
    return instance;
}

void DrawerRegistry::Register(TypeId type_id, PropertyDrawFunc drawer)
{
    drawers.Insert(type_id, drawer);
}

PropertyDrawFunc DrawerRegistry::Find(TypeId type_id) const
{
    if (const auto draw_fn = drawers.Find(type_id))
    {
        return *draw_fn;
    }
    return nullptr;
}

bool DrawerRegistry::DrawProperties(const TypeInfo& type_info, void* instance, bool read_only)
{
    const auto struct_info = type_info.AsStruct();
    if (!instance || !struct_info)
    {
        return false;
    }

    bool modified = false;
    u8* const bytes = static_cast<u8*>(instance);

    // 부모 클래스의 필드를 먼저 렌더링 (다중 상속 포함)
    for (const BaseInfo& base : struct_info->bases)
    {
        if (const auto base_info = TypeRegistry::Get().Find(base.type))
        {
            modified |= DrawProperties(*base_info, bytes + base.offset, read_only);
        }
    }

    for (const FieldInfo& field : struct_info->fields)
    {
        if (field.annotations.Has<display::HiddenAnnotation>())
        {
            continue;
        }

        void* const field_data = bytes + field.offset;

        // 부모와 자식의 필드 오프셋이 겹치지 않도록 주소로 ImGui ID를 구분
        ImGui::PushID(field_data);

        const auto display_name = field.annotations.Find<display::DisplayNameAnnotation>();
        const String label = display_name ? display_name->value : field.name;
        const bool field_read_only = read_only || field.annotations.Has<display::ReadOnlyAnnotation>();

        modified |= DrawValue(field.type, label.CStr(), field_data, field.annotations, field_read_only);

        ImGui::PopID();
    }

    return modified;
}

bool DrawerRegistry::DrawValue(
    TypeId type_id,
    const char* label,
    void* value,
    const AnnotationList& annotations,
    bool read_only
)
{
    // 등록된 Drawer가 있으면 사용
    if (const PropertyDrawFunc drawer = Find(type_id))
    {
        ImGui::BeginDisabled(read_only);
        const bool modified = drawer(label, value, annotations);
        ImGui::EndDisabled();
        return modified;
    }

    const auto type_info = TypeRegistry::Get().Find(type_id);
    if (!type_info)
    {
        ImGui::LabelText(label, "[unregistered %016llx]", static_cast<unsigned long long>(type_id.Value()));
        return false;
    }

    // 컨테이너와 Optional은 ValueOps가 없으면 타입 이름만 표시
    const auto draw_with_ops = [&]<typename Ops>(const auto& info, auto&& draw) -> bool
    {
        if (const auto ops = FindShapeOps<Ops>(type_id))
        {
            return draw(label, value, info, *ops, *this, read_only);
        }
        DrawTypeName(label, *type_info);
        return false;
    };

    return type_info->VisitShape(
        [&](const OpaqueInfo&) -> bool
        {
            DrawTypeName(label, *type_info);
            return false;
        },
        [&](const StructInfo& info) -> bool
        {
            if (info.bases.IsEmpty() && info.fields.IsEmpty())
            {
                DrawTypeName(label, *type_info);
                return false;
            }

            // 중첩 Struct -> TreeNode로 재귀 렌더링
            if (!ImGui::TreeNode(label))
            {
                return false;
            }
            const bool modified = DrawProperties(*type_info, value, read_only);
            ImGui::TreePop();
            return modified;
        },
        [&](const EnumInfo& info) -> bool
        {
            if (type_info->annotations.Has<display::BitFlagsAnnotation>())
            {
                // BitFlag Enum -> Checkbox 위젯
                return DrawBitFlags(label, value, *type_info, info, read_only);
            }

            // Enum -> Combo 위젯
            ImGui::BeginDisabled(read_only);
            const bool modified = DrawEnum(label, value, *type_info, info);
            ImGui::EndDisabled();
            return modified;
        },
        [&](const ArrayInfo& info) -> bool
        {
            return draw_with_ops.template operator()<ArrayOps>(info, &DrawArrayProperty);
        },
        [&](const SetInfo& info) -> bool
        {
            return draw_with_ops.template operator()<SetOps>(info, &DrawSetProperty);
        },
        [&](const MapInfo& info) -> bool
        {
            return draw_with_ops.template operator()<MapOps>(info, &DrawMapProperty);
        },
        [&](const OptionalInfo& info) -> bool
        {
            return draw_with_ops.template operator()<OptionalOps>(info, &DrawOptionalProperty);
        }
    );
}

void DrawerRegistry::RegisterBuiltinDrawers()
{
    // --- Primitive ---
    Register(TypeId::Of<bool>(),   &DrawBool);
    Register(TypeId::Of<i8>(),   &DrawArithmetic<i8>);
    Register(TypeId::Of<u8>(),  &DrawArithmetic<u8>);
    Register(TypeId::Of<i16>(),  &DrawArithmetic<i16>);
    Register(TypeId::Of<u16>(), &DrawArithmetic<u16>);
    Register(TypeId::Of<i32>(),  &DrawArithmetic<i32>);
    Register(TypeId::Of<u32>(), &DrawArithmetic<u32>);
    Register(TypeId::Of<i64>(),  &DrawArithmetic<i64>);
    Register(TypeId::Of<u64>(), &DrawArithmetic<u64>);
    Register(TypeId::Of<f32>(),  &DrawArithmetic<f32>);
    Register(TypeId::Of<f64>(), &DrawArithmetic<f64>);

    // --- String ---
    Register(TypeId::Of<String>(),      &DrawString);
    Register(TypeId::Of<StringName>(),  &DrawStringName);

    // --- Identifiers ---
    Register(TypeId::Of<Guid>(),        &DrawGuid);
    Register(TypeId::Of<TypeId>(),      &DrawTypeId);
    Register(TypeId::Of<AssetId>(),     &DrawAssetId);
    Register(TypeId::Of<Entity>(),      &DrawEntity);

    // --- Math (f64 precision) ---
    Register(TypeId::Of<Vector2>(),     &DrawVector2<f64>);
    Register(TypeId::Of<Vector3>(),     &DrawVector3<f64>);
    Register(TypeId::Of<Vector4>(),     &DrawVector4<f64>);
    Register(TypeId::Of<Quaternion>(),  &DrawQuaternion<f64>);
    Register(TypeId::Of<Rotator>(),     &DrawRotator<f64>);
    Register(TypeId::Of<Matrix4x4>(),   &DrawMatrix4x4<f64>);

    // --- Math (single precision) ---
    Register(TypeId::Of<Vector2f>(),    &DrawVector2<f32>);
    Register(TypeId::Of<Vector3f>(),    &DrawVector3<f32>);
    Register(TypeId::Of<Vector4f>(),    &DrawVector4<f32>);
    Register(TypeId::Of<Quaternionf>(), &DrawQuaternion<f32>);
    Register(TypeId::Of<Rotatorf>(),    &DrawRotator<f32>);
    Register(TypeId::Of<Matrix4x4f>(),  &DrawMatrix4x4<f32>);

    // --- Color ---
    Register(TypeId::Of<LinearColor>(), &DrawLinearColor);
    Register(TypeId::Of<Color>(),       &DrawColor);

    // --- Angles ---
    Register(TypeId::Of<Degree<f64>>(), &DrawDegree<f64>);
    Register(TypeId::Of<Degree<f32>>(),  &DrawDegree<f32>);
}
} // namespace se::editor
