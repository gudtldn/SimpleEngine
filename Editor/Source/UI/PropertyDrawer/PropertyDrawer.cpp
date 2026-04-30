#include "SimpleEditor/UI/PropertyDrawer/PropertyDrawer.h"

#include "UI/ImGui/ImGuiString.h"
#include "UI/ImGui/ImGuiWrapper.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/ECS/Entity.h"

#include "imgui.h"

#include <cstdio>


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
    if constexpr (std::same_as<T, int8>)        { return ImGuiDataType_S8;     }
    else if constexpr (std::same_as<T, uint8>)  { return ImGuiDataType_U8;     }
    else if constexpr (std::same_as<T, int16>)  { return ImGuiDataType_S16;    }
    else if constexpr (std::same_as<T, uint16>) { return ImGuiDataType_U16;    }
    else if constexpr (std::same_as<T, int32>)  { return ImGuiDataType_S32;    }
    else if constexpr (std::same_as<T, uint32>) { return ImGuiDataType_U32;    }
    else if constexpr (std::same_as<T, int64>)  { return ImGuiDataType_S64;    }
    else if constexpr (std::same_as<T, uint64>) { return ImGuiDataType_U64;    }
    else if constexpr (std::same_as<T, float>)  { return ImGuiDataType_Float;  }
    else if constexpr (std::same_as<T, double>) { return ImGuiDataType_Double; }
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

bool DrawBool(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    return ImGui::Checkbox(label, static_cast<bool*>(value));
}

// --- Arithmetic (int, uint, float, double) ---

template <typename T>
bool DrawArithmetic(const char* label, void* value, const PropertyInfo& prop)
{
    constexpr ImGuiDataType_ data_type = GetImGuiDataType<T>();
    T* v = static_cast<T*>(value);

    if (prop.metadata.flags.IsAnySet(EPropertyFlags::HasRange))
    {
        T min_val = static_cast<T>(prop.metadata.range_min);
        T max_val = static_cast<T>(prop.metadata.range_max);
        return ImGui::SliderScalar(label, data_type, v, &min_val, &max_val);
    }

    constexpr float speed = std::floating_point<T> ? 0.1f : 1.0f;

    if (prop.metadata.flags.IsAnySet(EPropertyFlags::HasClamp))
    {
        T min_val = static_cast<T>(prop.metadata.clamp_min);
        T max_val = static_cast<T>(prop.metadata.clamp_max);
        return ImGui::DragScalarNInfinity(label, data_type, v, 1, speed, &min_val, &max_val, nullptr, ImGuiSliderFlags_AlwaysClamp);
    }

    return ImGui::DragScalarNInfinity(label, data_type, v, 1, speed);
}

// --- String ---

bool DrawString(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    String& str = *static_cast<String*>(value);
    return ImGui::InputText(label, &str);
}

// --- StringName (read-only: interned string) ---

bool DrawStringName(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    const StringName& name = *static_cast<StringName*>(value);
    ImGui::LabelText(label, "%s", name.CStr());
    return false;
}

// --- Guid (read-only) ---

bool DrawGuid(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    const Guid& guid = *static_cast<Guid*>(value);
    const String str = guid.ToString();
    ImGui::LabelText(label, "%s", str.CStr());
    return false;
}

// --- TypeId (read-only) ---

bool DrawTypeId(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    const TypeId& type_id = *static_cast<TypeId*>(value);
    const StringView view = type_id.IsValid() ? type_id.GetName() : "(none)";
    ImGui::LabelText(label, "%.*s", static_cast<int>(view.ByteLen()), view.Data());
    return false;
}

// --- AssetId (GUID 표시 + Asset Drag&Drop Target) ---

bool DrawAssetId(const char* label, void* value, const PropertyInfo& /*prop*/)
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

bool DrawEntity(const char* label, void* value, const PropertyInfo& /*prop*/)
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
bool DrawVector2(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    using Vec = math::Vector2Impl<T>;
    constexpr ImGuiDataType_ data_type = GetImGuiDataType<T>();
    Vec* vec = static_cast<Vec*>(value);

    return ImGui::DragScalarNInfinity(label, data_type, &vec->x, 2, 0.1f);
}

// --- Vector3 / Vector3f ---

template <typename T>
bool DrawVector3(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    using Vec = math::Vector3Impl<T>;
    constexpr ImGuiDataType_ data_type = GetImGuiDataType<T>();
    Vec* vec = static_cast<Vec*>(value);

    return ImGui::DragScalarNInfinity(label, data_type, &vec->x, 3, 0.1f);
}

// --- Vector4 / Vector4f ---

template <typename T>
bool DrawVector4(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    using Vec = math::Vector4Impl<T>;
    constexpr ImGuiDataType_ data_type = GetImGuiDataType<T>();
    Vec* vec = static_cast<Vec*>(value);

    return ImGui::DragScalarNInfinity(label, data_type, &vec->x, 4, 0.1f);
}

// --- Quaternion / Quaternionf ---

template <typename T>
bool DrawQuaternion(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    using Quat = math::QuaternionImpl<T>;
    constexpr ImGuiDataType_ data_type = GetImGuiDataType<T>();
    Quat* quat = static_cast<Quat*>(value);
    return ImGui::DragScalarNInfinity(label, data_type, &quat->x, 4, 0.01f);
}

// --- Rotator / Rotatorf ---

template <typename T>
bool DrawRotator(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    using Rot = math::RotatorImpl<T>;
    constexpr ImGuiDataType_ data_type = GetImGuiDataType<T>();
    Rot* rot = static_cast<Rot*>(value);

    // pitch, roll, yaw — 각각 Degree<T>이므로 .value 멤버에 직접 접근
    return ImGui::DragScalarNInfinity(label, data_type, &rot->pitch.value, 3, 0.1f);
}

// --- Matrix4x4 / Matrix4x4f ---
template <typename T>
bool DrawMatrix4x4(const char* label, void* value, const PropertyInfo& prop)
{
    using Mat = math::Matrix4x4Impl<T>;
    constexpr ImGuiDataType_ data_type = GetImGuiDataType<T>();
    Mat* mat = static_cast<Mat*>(value);

    bool modified = false;

    const bool read_only = prop.metadata.flags.IsAnySet(EPropertyFlags::ReadOnly);

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

            modified |= ImGui::DragScalarNInfinity(row_label, data_type, row_ptr, 4, 0.01f);

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

bool DrawLinearColor(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    LinearColor* color = static_cast<LinearColor*>(value);
    return ImGui::ColorEdit4(label, &color->r);
}

// --- Color (uint8 RGBA) ---

bool DrawColor(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    Color* color = static_cast<Color*>(value);
    float rgba[4] = {
        static_cast<float>(color->r) / 255.0f,
        static_cast<float>(color->g) / 255.0f,
        static_cast<float>(color->b) / 255.0f,
        static_cast<float>(color->a) / 255.0f,
    };

    if (ImGui::ColorEdit4(label, rgba))
    {
        color->r = math::RoundToInt<uint8>(rgba[0] * 255.0f);
        color->g = math::RoundToInt<uint8>(rgba[1] * 255.0f);
        color->b = math::RoundToInt<uint8>(rgba[2] * 255.0f);
        color->a = math::RoundToInt<uint8>(rgba[3] * 255.0f);
        return true;
    }
    return false;
}

// --- Degree<T> (AngleType) ---

template <typename T>
bool DrawDegree(const char* label, void* value, const PropertyInfo& prop)
{
    using Deg = Degree<T>;
    constexpr ImGuiDataType_ data_type = GetImGuiDataType<T>();
    Deg* angle = static_cast<Deg*>(value);

    if (prop.metadata.flags.IsAnySet(EPropertyFlags::HasRange))
    {
        T min_val = static_cast<T>(prop.metadata.range_min);
        T max_val = static_cast<T>(prop.metadata.range_max);
        return ImGui::SliderScalar(label, data_type, &angle->value, &min_val, &max_val);
    }

    return ImGui::DragScalarNInfinity(label, data_type, &angle->value, 1, 0.1f);
}

// ============================================================================
// Enum Helpers
// ============================================================================

/** type-erased enum 값을 int64로 읽기 (signed/unsigned 대응) */
int64 ReadEnumValue(const void* value, usize size, bool is_unsigned)
{
    if (is_unsigned)
    {
        switch (size)
        {
        case sizeof(uint8):
            return static_cast<int64>(*static_cast<const uint8*>(value));
        case sizeof(uint16):
            return static_cast<int64>(*static_cast<const uint16*>(value));
        case sizeof(uint32):
            return static_cast<int64>(*static_cast<const uint32*>(value));
        case sizeof(uint64):
            return static_cast<int64>(*static_cast<const uint64*>(value));
        default:
            break;
        }
    }
    else
    {
        switch (size)
        {
        case sizeof(int8):
            return static_cast<int64>(*static_cast<const int8*>(value));
        case sizeof(int16):
            return static_cast<int64>(*static_cast<const int16*>(value));
        case sizeof(int32):
            return static_cast<int64>(*static_cast<const int32*>(value));
        case sizeof(int64):
            return *static_cast<const int64*>(value);
        default:
            break;
        }
    }
    return 0;
}

/** type-erased enum에 int64 값 쓰기 (signed/unsigned 대응) */
void WriteEnumValue(void* value, int64 new_value, usize size, bool is_unsigned)
{
    if (is_unsigned)
    {
        switch (size)
        {
        case sizeof(uint8):
            *static_cast<uint8*>(value) = static_cast<uint8>(new_value);
            break;
        case sizeof(uint16):
            *static_cast<uint16*>(value) = static_cast<uint16>(new_value);
            break;
        case sizeof(uint32):
            *static_cast<uint32*>(value) = static_cast<uint32>(new_value);
            break;
        case sizeof(uint64):
            *static_cast<uint64*>(value) = static_cast<uint64>(new_value);
            break;
        default: break;
        }
    }
    else
    {
        switch (size)
        {
        case sizeof(int8):
            *static_cast<int8*>(value) = static_cast<int8>(new_value);
            break;
        case sizeof(int16):
            *static_cast<int16*>(value) = static_cast<int16>(new_value);
            break;
        case sizeof(int32):
            *static_cast<int32*>(value) = static_cast<int32>(new_value);
            break;
        case sizeof(int64):
            *static_cast<int64*>(value) = new_value;
            break;
        default:
            break;
        }
    }
}

// ============================================================================
// Enum Drawer (generic, type-erased)
// ============================================================================

bool DrawEnum(const char* label, void* value, const PropertyInfo& prop)
{
    const Optional type_info_opt = TypeRegistry::Get().Find(prop.type_id);
    SE_ASSERT(
        type_info_opt && type_info_opt->enum_entries,
        "Enum '{}' is registered without enum_entries. Use SE_REFLECT_ENUM to register.", prop.type_id.GetName()
    );

    const EnumEntry* entries = nullptr;
    usize count = 0;
    type_info_opt->enum_entries(entries, count);

    if (count == 0)
    {
        ImGui::LabelText(label, "[Empty enum]");
        return false;
    }

    const bool is_unsigned = type_info_opt->flags.IsAnySet(ETypeFlags::IsUnsigned);
    const int64 current_value = ReadEnumValue(value, type_info_opt->size, is_unsigned);

    // 현재 선택 인덱스 찾기
    usize current_idx = count; // invalid sentinel
    for (usize i = 0; i < count; ++i)
    {
        if (entries[i].value == current_value)
        {
            current_idx = i;
            break;
        }
    }

    // TODO: 나중에 최적화 하려면 LinearAllocator로 최적화
    const String preview = (current_idx < count) ? entries[current_idx].name : "???";

    bool modified = false;
    if (ImGui::BeginCombo(label, preview.CStr()))
    {
        for (usize i = 0; i < count; ++i)
        {
            const String entry_name = entries[i].name;
            const bool is_selected = (i == current_idx);
            if (ImGui::Selectable(entry_name.CStr(), is_selected))
            {
                WriteEnumValue(value, entries[i].value, type_info_opt->size, is_unsigned);
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

bool DrawBitFlags(const char* label, void* value, const PropertyInfo& prop)
{
    const Optional type_info_opt = TypeRegistry::Get().Find(prop.type_id);
    SE_ASSERT(
        type_info_opt && type_info_opt->enum_entries,
        "BitFlag enum '{}' is registered without enum_entries. Use SE_REFLECT_ENUM to register.", prop.type_id.GetName()
    );

    const EnumEntry* entries = nullptr;
    usize count = 0;
    type_info_opt->enum_entries(entries, count);

    if (count == 0)
    {
        ImGui::LabelText(label, "[Empty bitflag]");
        return false;
    }

    const bool is_unsigned = type_info_opt->flags.IsAnySet(ETypeFlags::IsUnsigned);
    int64 current_value = ReadEnumValue(value, type_info_opt->size, is_unsigned);

    bool modified = false;
    if (ImGui::TreeNode(label))
    {
        for (usize i = 0; i < count; ++i)
        {
            const int64 flag = entries[i].value;
            bool has_flag = (current_value & flag) == flag;

            const String entry_name = entries[i].name;
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

        if (modified)
        {
            WriteEnumValue(value, current_value, type_info_opt->size, is_unsigned);
        }

        ImGui::TreePop();
    }
    return modified;
}

// ============================================================================
// Container Drawer Helpers
// ============================================================================

bool DrawArrayContent(const ContainerOps& ops, void* container, DrawerRegistry& registry, bool read_only)
{
    bool modified = false;
    const usize count = ops.size(container);

    struct IterState
    {
        DrawerRegistry* registry;
        TypeId elem_type_id;
        const ContainerOps* elem_container_ops;
        const OptionalOps* elem_optional_ops;
        bool read_only;
        bool modified;
        usize remove_idx;
        usize count;
    };

    IterState state = {
        .registry = &registry,
        .elem_type_id = ops.element_type_id,
        .elem_container_ops = ops.element_container_ops,
        .elem_optional_ops = ops.element_optional_ops,
        .read_only = read_only,
        .modified = false,
        .remove_idx = count,
        .count = count
    };

    ops.for_each(container, [](usize idx, void* elem, void* /*unused*/, void* user) -> bool
    {
        IterState& s = *static_cast<IterState*>(user);

        ImGui::PushID(static_cast<int>(idx));

        // [×] 삭제 버튼
        if (!s.read_only)
        {
            if (ImGui::SmallButton("x"))
            {
                s.remove_idx = idx;
                s.modified = true;
            }
            ImGui::SameLine();
        }

        // 인덱스 라벨 + 요소 렌더링
        char label[32];
        std::snprintf(label, sizeof(label), "[%zu]", idx);
        s.modified |= s.registry->DrawValue(s.elem_type_id, label, elem, s.elem_container_ops, s.elem_optional_ops);

        ImGui::PopID();
        return true;
    }, &state);

    modified = state.modified;

    // 지연 삭제
    if (state.remove_idx < count)
    {
        ops.remove_at(container, state.remove_idx);
    }

    return modified;
}

bool DrawSetContent(const ContainerOps& ops, void* container, DrawerRegistry& registry, bool read_only)
{
    bool modified = false;
    const usize count = ops.size(container);

    struct IterState
    {
        DrawerRegistry* registry;
        TypeId elem_type_id;
        const ContainerOps* elem_container_ops;
        const OptionalOps* elem_optional_ops;
        bool read_only;
        bool modified;
        usize remove_idx;
        usize count;
    };

    IterState state = {
        .registry = &registry,
        .elem_type_id = ops.element_type_id,
        .elem_container_ops = ops.element_container_ops,
        .elem_optional_ops = ops.element_optional_ops,
        .read_only = read_only,
        .modified = false,
        .remove_idx = count,
        .count = count
    };

    ops.for_each(container, [](usize idx, void* elem, void* /*unused*/, void* user) -> bool
    {
        IterState& s = *static_cast<IterState*>(user);

        ImGui::PushID(static_cast<int>(idx));

        // [×] 삭제 버튼
        if (!s.read_only)
        {
            if (ImGui::SmallButton("x"))
            {
                s.remove_idx = idx;
                s.modified = true;
            }
            ImGui::SameLine();
        }

        char label[32];
        std::snprintf(label, sizeof(label), "[%zu]", idx);

        // Set 요소는 읽기 전용 (값을 변경하면 해시가 깨짐)
        ImGui::BeginDisabled();
        s.registry->DrawValue(s.elem_type_id, label, elem, s.elem_container_ops, s.elem_optional_ops);
        ImGui::EndDisabled();

        ImGui::PopID();
        return true;
    }, &state);

    modified = state.modified;

    // 지연 삭제
    if (state.remove_idx < count)
    {
        ops.remove_at(container, state.remove_idx);
        modified = true;
    }

    return modified;
}

bool DrawMapContent(const ContainerOps& ops, void* container, DrawerRegistry& registry, bool read_only)
{
    bool modified = false;
    const usize count = ops.size(container);

    struct IterState
    {
        DrawerRegistry* registry;
        TypeId key_type_id;
        TypeId value_type_id;
        const ContainerOps* key_container_ops;
        const OptionalOps* key_optional_ops;
        const ContainerOps* value_container_ops;
        const OptionalOps* value_optional_ops;
        bool read_only;
        bool modified;
        usize remove_idx;
        usize count;
    };

    IterState state = {
        .registry = &registry,
        .key_type_id = ops.element_type_id, .value_type_id = ops.value_type_id,
        .key_container_ops = ops.element_container_ops, .key_optional_ops = ops.element_optional_ops,
        .value_container_ops = ops.value_container_ops, .value_optional_ops = ops.value_optional_ops,
        .read_only = read_only,
        .modified = false,
        .remove_idx = count,
        .count = count
    };

    ops.for_each(container, [](usize idx, void* key, void* value, void* user) -> bool
    {
        IterState& s = *static_cast<IterState*>(user);

        ImGui::PushID(static_cast<int>(idx));

        char entry_label[32];
        std::snprintf(entry_label, sizeof(entry_label), "[%zu]", idx);

        // [×] 삭제 버튼을 TreeNode 앞에 배치
        bool want_remove = false;
        if (!s.read_only)
        {
            if (ImGui::SmallButton("x"))
            {
                s.remove_idx = idx;
                s.modified = true;
                want_remove = true;
            }
            ImGui::SameLine();
        }

        if (!want_remove && ImGui::TreeNode(entry_label))
        {
            // Key는 읽기 전용 (변경하면 해시가 깨짐)
            ImGui::BeginDisabled();
            s.registry->DrawValue(s.key_type_id, "Key", key, s.key_container_ops, s.key_optional_ops);
            ImGui::EndDisabled();

            // Value는 편집 가능
            s.modified |= s.registry->DrawValue(s.value_type_id, "Value", value, s.value_container_ops, s.value_optional_ops);

            ImGui::TreePop();
        }

        ImGui::PopID();
        return true;
    }, &state);

    modified = state.modified;

    // 지연 삭제
    if (state.remove_idx < count)
    {
        ops.remove_at(container, state.remove_idx);
        modified = true;
    }

    return modified;
}

bool DrawContainerProperty(
    const char* label,
    void* container,
    const ContainerOps& ops,
    DrawerRegistry& registry,
    bool read_only
)
{
    const usize count = ops.size(container);

    // 헤더: "label (N elements)"
    char header[256];
    std::snprintf(header, sizeof(header), "%s (%zu)", label, count);

    bool modified = false;
    if (ImGui::TreeNode(header))
    {
        // [+] [Clear] 버튼
        if (!read_only)
        {
            if (ops.add)
            {
                if (ImGui::SmallButton("+"))
                {
                    ops.add(container);
                    modified = true;
                }
                ImGui::SameLine();
            }

            if (count > 0)
            {
                if (ImGui::SmallButton("Clear"))
                {
                    ops.clear(container);
                    modified = true;
                    ImGui::TreePop();
                    return modified;
                }
            }
        }

        // 요소 렌더링
        switch (ops.kind)
        {
        case EContainerKind::Array:
            modified |= DrawArrayContent(ops, container, registry, read_only);
            break;
        case EContainerKind::Set:
            modified |= DrawSetContent(ops, container, registry, read_only);
            break;
        case EContainerKind::Map:
            modified |= DrawMapContent(ops, container, registry, read_only);
            break;
        default:
            break;
        }

        ImGui::TreePop();
    }

    return modified;
}

bool DrawOptionalProperty(
    const char* label,
    void* optional,
    const OptionalOps& ops,
    DrawerRegistry& registry,
    bool read_only
)
{
    bool modified = false;
    bool has_value = ops.has_value(optional);

    // "label" TreeNode 또는 인라인 Checkbox로 표시
    if (ImGui::TreeNode(label))
    {
        // Checkbox: 값 존재 여부 토글
        if (!read_only && ops.emplace_default)
        {
            if (ImGui::Checkbox("Has Value", &has_value))
            {
                if (has_value)
                {
                    ops.emplace_default(optional);
                }
                else
                {
                    ops.reset(optional);
                }
                modified = true;
            }
        }
        else
        {
            // 기본 생성 불가능하거나 ReadOnly인 경우 표시만
            ImGui::BeginDisabled();
            ImGui::Checkbox("Has Value", &has_value);
            ImGui::EndDisabled();
        }

        // 값이 있으면 내부 값 렌더링
        if (has_value)
        {
            void* inner_value = ops.get_value(optional);

            if (read_only)
            {
                ImGui::BeginDisabled();
            }

            modified |= registry.DrawValue(
                ops.inner_type_id, "Value", inner_value,
                ops.inner_container_ops, ops.inner_optional_ops
            );

            if (read_only)
            {
                ImGui::EndDisabled();
            }
        }

        ImGui::TreePop();
    }

    return modified;
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

void DrawerRegistry::Register(const TypeId& type_id, PropertyDrawFunc drawer)
{
    drawers.Insert(type_id, drawer);
}

PropertyDrawFunc DrawerRegistry::Find(const TypeId& type_id) const
{
    if (const Optional opt = drawers.Find(type_id))
    {
        return *opt;
    }
    return nullptr;
}

bool DrawerRegistry::DrawProperties(const TypeInfo& type_info, void* instance)
{
    bool modified = false;

    // 부모 클래스의 프로퍼티를 먼저 렌더링
    if (type_info.base_or_inner_id.IsValid() && type_info.kind == ETypeKind::Struct)
    {
        if (const Optional parent_opt = TypeRegistry::Get().Find(type_info.base_or_inner_id))
        {
            modified |= DrawProperties(*parent_opt, instance);
        }
    }

    for (const PropertyInfo& prop : type_info.properties)
    {
        // Hidden 프로퍼티는 건너뛰기
        if (prop.metadata.flags.IsAnySet(EPropertyFlags::Hidden))
        {
            continue;
        }

        // 고유 ImGui ID 보장 (같은 이름 충돌 방지)
        ImGui::PushID(static_cast<int>(prop.offset));

        void* prop_data = prop.accessor.get_ptr(instance);

        // ReadOnly면 ImGui 위젯 비활성화
        const bool read_only = prop.metadata.flags.IsAnySet(EPropertyFlags::ReadOnly);
        if (read_only)
        {
            ImGui::BeginDisabled();
        }

        // 표시 이름 결정
        const char* label = prop.metadata.display_name.IsEmpty()
                                ? prop.name.Data()
                                : prop.metadata.display_name.Data();

        // 컨테이너 프로퍼티 (ContainerOps가 설정된 경우)
        if (prop.container_ops)
        {
            // 컨테이너는 ReadOnly를 자체적으로 처리 (BeginDisabled 중첩 방지)
            if (read_only)
            {
                ImGui::EndDisabled();
            }

            modified |= DrawContainerProperty(label, prop_data, *prop.container_ops, *this, read_only);

            if (read_only)
            {
                ImGui::BeginDisabled();
            }
        }

        // Optional 프로퍼티 (OptionalOps가 설정된 경우)
        else if (prop.optional_ops)
        {
            // Optional도 ReadOnly를 자체적으로 처리 (BeginDisabled 중첩 방지)
            if (read_only)
            {
                ImGui::EndDisabled();
            }

            modified |= DrawOptionalProperty(label, prop_data, *prop.optional_ops, *this, read_only);

            if (read_only)
            {
                ImGui::BeginDisabled();
            }
        }

        // 등록된 Drawer가 있으면 사용
        else if (const PropertyDrawFunc drawer = Find(prop.type_id))
        {
            modified |= drawer(label, prop_data, prop);
        }

        // TypeRegistry에서 타입 정보 조회하여 분기
        else if (Optional prop_type_opt = TypeRegistry::Get().Find(prop.type_id))
        {
            if (prop_type_opt->kind == ETypeKind::Enum && prop_type_opt->enum_entries)
            {
                if (prop_type_opt->flags.IsAnySet(ETypeFlags::IsBitFlag))
                {
                    // BitFlag Enum -> Checkbox 위젯
                    modified |= DrawBitFlags(label, prop_data, prop);
                }
                else
                {
                    // Enum -> Combo 위젯
                    modified |= DrawEnum(label, prop_data, prop);
                }
            }
            else if (prop_type_opt->kind == ETypeKind::Struct && !prop_type_opt->properties.IsEmpty())
            {
                // 중첩 Struct -> TreeNode로 재귀 렌더링
                if (ImGui::TreeNode(label))
                {
                    modified |= DrawProperties(*prop_type_opt, prop_data);
                    ImGui::TreePop();
                }
            }
            else
            {
                const StringView view = prop.type_id.GetName();
                ImGui::LabelText(label, "[%.*s]", static_cast<int>(view.ByteLen()), view.Data());
            }
        }
        else
        {
            // TypeRegistry에 미등록된 타입
            const StringView view = prop.type_id.GetName();
            ImGui::LabelText(label, "[%.*s - unregistered]", static_cast<int>(view.ByteLen()), view.Data());
        }

        if (read_only)
        {
            ImGui::EndDisabled();
        }

        // Tooltip
        if (!prop.metadata.tooltip.IsEmpty() && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%.*s", static_cast<int>(prop.metadata.tooltip.ByteLen()), prop.metadata.tooltip.Data());
        }

        ImGui::PopID();
    }

    return modified;
}

bool DrawerRegistry::DrawValue(
    const TypeId& type_id,
    const char* label,
    void* value,
    const ContainerOps* container_ops,
    const OptionalOps* optional_ops
)
{
    // 컨테이너 타입: 중첩 ContainerOps를 통해 렌더링
    if (container_ops)
    {
        return DrawContainerProperty(label, value, *container_ops, *this, false);
    }

    // Optional 타입: 중첩 OptionalOps를 통해 렌더링
    if (optional_ops)
    {
        return DrawOptionalProperty(label, value, *optional_ops, *this, false);
    }

    // 등록된 Drawer가 있으면 사용
    if (const PropertyDrawFunc drawer = Find(type_id))
    {
        // 빈 PropertyInfo 생성 (메타데이터 없음)
        PropertyInfo dummy_prop;
        dummy_prop.type_id = type_id;
        return drawer(label, value, dummy_prop);
    }

    // TypeRegistry에서 타입 정보 조회하여 분기
    if (const Optional type_opt = TypeRegistry::Get().Find(type_id))
    {
        if (type_opt->kind == ETypeKind::Enum && type_opt->enum_entries)
        {
            PropertyInfo dummy_prop;
            dummy_prop.type_id = type_id;

            if (type_opt->flags.IsAnySet(ETypeFlags::IsBitFlag))
            {
                return DrawBitFlags(label, value, dummy_prop);
            }
            return DrawEnum(label, value, dummy_prop);
        }

        if (type_opt->kind == ETypeKind::Struct && !type_opt->properties.IsEmpty())
        {
            if (ImGui::TreeNode(label))
            {
                const bool modified = DrawProperties(*type_opt, value);
                ImGui::TreePop();
                return modified;
            }
            return false;
        }
    }

    // 지원하지 않는 타입은 타입명만 표시
    const StringView view = type_id.GetName();
    ImGui::LabelText(label, "[%.*s]", static_cast<int>(view.ByteLen()), view.Data());
    return false;
}

void DrawerRegistry::RegisterBuiltinDrawers()
{
    // --- Primitive ---
    Register(TypeId::Get<bool>(),   &DrawBool);
    Register(TypeId::Get<int8>(),   &DrawArithmetic<int8>);
    Register(TypeId::Get<uint8>(),  &DrawArithmetic<uint8>);
    Register(TypeId::Get<int16>(),  &DrawArithmetic<int16>);
    Register(TypeId::Get<uint16>(), &DrawArithmetic<uint16>);
    Register(TypeId::Get<int32>(),  &DrawArithmetic<int32>);
    Register(TypeId::Get<uint32>(), &DrawArithmetic<uint32>);
    Register(TypeId::Get<int64>(),  &DrawArithmetic<int64>);
    Register(TypeId::Get<uint64>(), &DrawArithmetic<uint64>);
    Register(TypeId::Get<float>(),  &DrawArithmetic<float>);
    Register(TypeId::Get<double>(), &DrawArithmetic<double>);

    // --- String ---
    Register(TypeId::Get<String>(),      &DrawString);
    Register(TypeId::Get<StringName>(),  &DrawStringName);

    // --- Identifiers ---
    Register(TypeId::Get<Guid>(),        &DrawGuid);
    Register(TypeId::Get<TypeId>(),      &DrawTypeId);
    Register(TypeId::Get<AssetId>(),     &DrawAssetId);
    Register(TypeId::Get<Entity>(),      &DrawEntity);

    // --- Math (double precision) ---
    Register(TypeId::Get<Vector2>(),     &DrawVector2<double>);
    Register(TypeId::Get<Vector3>(),     &DrawVector3<double>);
    Register(TypeId::Get<Vector4>(),     &DrawVector4<double>);
    Register(TypeId::Get<Quaternion>(),  &DrawQuaternion<double>);
    Register(TypeId::Get<Rotator>(),     &DrawRotator<double>);
    Register(TypeId::Get<Matrix4x4>(),   &DrawMatrix4x4<double>);

    // --- Math (single precision) ---
    Register(TypeId::Get<Vector2f>(),    &DrawVector2<float>);
    Register(TypeId::Get<Vector3f>(),    &DrawVector3<float>);
    Register(TypeId::Get<Vector4f>(),    &DrawVector4<float>);
    Register(TypeId::Get<Quaternionf>(), &DrawQuaternion<float>);
    Register(TypeId::Get<Rotatorf>(),    &DrawRotator<float>);
    Register(TypeId::Get<Matrix4x4f>(),  &DrawMatrix4x4<float>);

    // --- Color ---
    Register(TypeId::Get<LinearColor>(), &DrawLinearColor);
    Register(TypeId::Get<Color>(),       &DrawColor);

    // --- Angles ---
    Register(TypeId::Get<Degree<double>>(), &DrawDegree<double>);
    Register(TypeId::Get<Degree<float>>(),  &DrawDegree<float>);
}
} // namespace se::editor
