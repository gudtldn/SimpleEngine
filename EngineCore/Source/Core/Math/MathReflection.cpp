#include "SimpleEngine/Core/Math/MathReflection.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Serialization/Archive.h"
#include "SimpleEngine/Core/Serialization/SerializeOpsRegistry.h"


namespace se
{
/** 도 단위 숫자 하나로 저장합니다. 예: fov = 90.0 */
template <>
struct SerializeTraits<Degree<f64>>
{
    static constexpr u32 FORMAT_VERSION = 1;

    static void Write(ArchiveWriter& writer, const Degree<f64>& value)
    {
        writer.Float(value.value, serde::FloatWidthOf<f64>());
    }

    static void Read(ArchiveReader& reader, Degree<f64>& value)
    {
        f64 degrees = 0.0;
        reader.Float(degrees, serde::FloatWidthOf<f64>());
        if (reader.HasError())
        {
            return;
        }
        value = Degree<f64>{ degrees };
    }
};
} // namespace se

SE_REGISTER_SERIALIZE_TRAITS(se::Degree<f64>)


SE_REFLECT_BEGIN(se::Vector3)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::Quaternion)
    SE_FIELD(x)
    SE_FIELD(y)
    SE_FIELD(z)
    SE_FIELD(w)
SE_REFLECT_END()
