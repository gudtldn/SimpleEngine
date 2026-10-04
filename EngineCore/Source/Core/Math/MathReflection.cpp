#include "SimpleEngine/Core/Math/MathReflection.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"


namespace se
{
/** 각도를 자기 단위의 숫자 하나로 저장합니다. 예: fov = 90.0 */
template <traits::FloatingType T, typename UnitTag>
struct SerializeTraits<math::AngleType<T, UnitTag>>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const math::AngleType<T, UnitTag>& value)
    {
        writer.Float(value.value, serde::FloatWidthOf<T>());
    }

    static void Read(ArchiveReader& reader, math::AngleType<T, UnitTag>& value)
    {
        f64 number = 0.0;
        reader.Float(number, serde::FloatWidthOf<T>());
        if (reader.HasError())
        {
            return;
        }
        value = math::AngleType<T, UnitTag>{ static_cast<T>(number) };
    }
};
} // namespace se

SE_REGISTER_SERIALIZE_TRAITS(se::Degree<f64>)
SE_REGISTER_SERIALIZE_TRAITS(se::Degree<f32>)
SE_REGISTER_SERIALIZE_TRAITS(se::Radian<f64>)
SE_REGISTER_SERIALIZE_TRAITS(se::Radian<f32>)

SE_REFLECT_BEGIN(se::Vector2)
    SE_FIELD(x)
    SE_FIELD(y)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Vector2f)
    SE_FIELD(x)
    SE_FIELD(y)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Vector3)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Vector3f)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Vector4)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
    SE_FIELD(w)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Vector4f)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
    SE_FIELD(w)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Quaternion)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
    SE_FIELD(w)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Quaternionf)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
    SE_FIELD(w)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Rotator)
    SE_FIELD(pitch)
    SE_FIELD(roll)
    SE_FIELD(yaw)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Rotatorf)
    SE_FIELD(pitch)
    SE_FIELD(roll)
    SE_FIELD(yaw)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Matrix4x4)
    SE_FIELD(data)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Matrix4x4f)
    SE_FIELD(data)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::AABB)
    SE_FIELD(min)
    SE_FIELD(max)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::AABBf)
    SE_FIELD(min)
    SE_FIELD(max)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Color)
    SE_FIELD(r)
    SE_FIELD(g)
    SE_FIELD(b)
    SE_FIELD(a)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::LinearColor)
    SE_FIELD(r)
    SE_FIELD(g)
    SE_FIELD(b)
    SE_FIELD(a)
SE_REFLECT_END()
