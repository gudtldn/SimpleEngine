#include "UI/PropertyDrawer/PropertyDrawer.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Math/Math.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"
#include "SimpleEngine/ECS/Entity.h"

#include "imgui.h"


namespace
{
using namespace se;

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
    constexpr auto data_type = GetImGuiDataType<T>();
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
        return ImGui::DragScalar(label, data_type, v, speed, &min_val, &max_val, nullptr, ImGuiSliderFlags_AlwaysClamp);
    }

    return ImGui::DragScalar(label, data_type, v, speed);
}

// --- String ---

bool DrawString(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    String& str = *static_cast<String*>(value);

    char buf[1024];
    const usize len = std::min(str.ByteLen(), sizeof(buf) - 1);
    std::memcpy(buf, str.CStr(), len);
    buf[len] = '\0';

    if (ImGui::InputText(label, buf, sizeof(buf)))
    {
        str = String(buf);
        return true;
    }
    return false;
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

// --- AssetId (read-only: GUID 표시) ---

bool DrawAssetId(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    const asset::AssetId& asset_id = *static_cast<asset::AssetId*>(value);
    if (asset_id.IsValid())
    {
        const String str = asset_id.GetGuid().ToString();
        ImGui::LabelText(label, "%s", str.CStr());
    }
    else
    {
        ImGui::LabelText(label, "(none)");
    }
    return false;
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
    constexpr auto data_type = GetImGuiDataType<T>();
    auto* vec = static_cast<Vec*>(value);

    return ImGui::DragScalarN(label, data_type, &vec->x, 2, 0.1f);
}

// --- Vector3 / Vector3f ---

template <typename T>
bool DrawVector3(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    using Vec = math::Vector3Impl<T>;
    constexpr auto data_type = GetImGuiDataType<T>();
    auto* vec = static_cast<Vec*>(value);

    return ImGui::DragScalarN(label, data_type, &vec->x, 3, 0.1f);
}

// --- Vector4 / Vector4f ---

template <typename T>
bool DrawVector4(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    using Vec = math::Vector4Impl<T>;
    constexpr auto data_type = GetImGuiDataType<T>();
    auto* vec = static_cast<Vec*>(value);

    return ImGui::DragScalarN(label, data_type, &vec->x, 4, 0.1f);
}

// --- Quaternion / Quaternionf ---

template <typename T>
bool DrawQuaternion(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    using Quat = math::QuaternionImpl<T>;
    constexpr auto data_type = GetImGuiDataType<T>();
    auto* quat = static_cast<Quat*>(value);
    return ImGui::DragScalarN(label, data_type, &quat->x, 4, 0.01f);
}

// --- Rotator / Rotatorf ---

template <typename T>
bool DrawRotator(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    using Rot = math::RotatorImpl<T>;
    constexpr auto data_type = GetImGuiDataType<T>();
    auto* rot = static_cast<Rot*>(value);

    // pitch, yaw, roll — 각각 Degree<T>이므로 .value 멤버에 직접 접근
    T angles[3] = { rot->pitch.value, rot->yaw.value, rot->roll.value };
    if (ImGui::DragScalarN(label, data_type, angles, 3, 0.1f))
    {
        rot->pitch.value = angles[0];
        rot->yaw.value   = angles[1];
        rot->roll.value  = angles[2];
        return true;
    }
    return false;
}

// --- LinearColor ---

bool DrawLinearColor(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    auto* color = static_cast<math::LinearColor*>(value);
    return ImGui::ColorEdit4(label, &color->r);
}

// --- Color (uint8 RGBA) ---

bool DrawColor(const char* label, void* value, const PropertyInfo& /*prop*/)
{
    auto* color = static_cast<math::Color*>(value);
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
    constexpr auto data_type = GetImGuiDataType<T>();
    auto* angle = static_cast<Deg*>(value);

    if (prop.metadata.flags.IsAnySet(EPropertyFlags::HasRange))
    {
        T min_val = static_cast<T>(prop.metadata.range_min);
        T max_val = static_cast<T>(prop.metadata.range_max);
        return ImGui::SliderScalar(label, data_type, &angle->value, &min_val, &max_val);
    }

    return ImGui::DragScalar(label, data_type, &angle->value, 0.1f);
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

    const char* preview = (current_idx < count) ? entries[current_idx].name.Data() : "???";

    bool modified = false;
    if (ImGui::BeginCombo(label, preview))
    {
        for (usize i = 0; i < count; ++i)
        {
            const bool is_selected = (i == current_idx);
            if (ImGui::Selectable(entries[i].name.Data(), is_selected))
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

            if (ImGui::Checkbox(entries[i].name.Data(), &has_flag))
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
} // namespace

namespace se::editor
{
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
    if (auto opt = drawers.Find(type_id))
    {
        return *opt;
    }
    return nullptr;
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
    Register(TypeId::Get<String>(),     &DrawString);
    Register(TypeId::Get<StringName>(), &DrawStringName);

    // --- Identifiers ---
    Register(TypeId::Get<Guid>(),           &DrawGuid);
    Register(TypeId::Get<TypeId>(),         &DrawTypeId);
    Register(TypeId::Get<asset::AssetId>(), &DrawAssetId);
    Register(TypeId::Get<Entity>(),         &DrawEntity);

    // --- Math (double precision) ---
    Register(TypeId::Get<Vector2>(),    &DrawVector2<double>);
    Register(TypeId::Get<Vector3>(),    &DrawVector3<double>);
    Register(TypeId::Get<Vector4>(),    &DrawVector4<double>);
    Register(TypeId::Get<Quaternion>(), &DrawQuaternion<double>);
    Register(TypeId::Get<Rotator>(),    &DrawRotator<double>);

    // --- Math (single precision) ---
    Register(TypeId::Get<Vector2f>(),    &DrawVector2<float>);
    Register(TypeId::Get<Vector3f>(),    &DrawVector3<float>);
    Register(TypeId::Get<Vector4f>(),    &DrawVector4<float>);
    Register(TypeId::Get<Quaternionf>(), &DrawQuaternion<float>);
    Register(TypeId::Get<Rotatorf>(),    &DrawRotator<float>);

    // --- Color ---
    Register(TypeId::Get<LinearColor>(), &DrawLinearColor);
    Register(TypeId::Get<Color>(),       &DrawColor);

    // --- Angles ---
    Register(TypeId::Get<Degree<double>>(), &DrawDegree<double>);
    Register(TypeId::Get<Degree<float>>(),  &DrawDegree<float>);
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

        // 등록된 Drawer가 있으면 사용
        if (const PropertyDrawFunc drawer = Find(prop.type_id))
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
} // namespace se::editor
