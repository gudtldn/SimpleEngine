#include "SimpleEngine/Graphics/MeshPrimitives.h"

#include "SimpleEngine/Core/Math/MathReflection.h"
#include "SimpleEngine/Core/Reflection/ReflectMacros.h"


// 필드는 멤버 선언 순서(오프셋 순서)로 등록
SE_REFLECT_BEGIN(se::StaticVertex)
    SE_FIELD(position)
    SE_FIELD(normal)
    SE_FIELD(tex_coord)
    SE_FIELD(tangent)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::SkinVertex)
    SE_FIELD(bone_indices)
    SE_FIELD(bone_weights)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::MeshSection)
    SE_FIELD(index_offset)
    SE_FIELD(index_count)
    SE_FIELD(vertex_offset)
    SE_FIELD(vertex_count)
    SE_FIELD(material_slot)
    SE_FIELD(bounds)
SE_REFLECT_END()

SE_REFLECT_BEGIN(se::MeshLOD)
    SE_FIELD(screen_size)
    SE_FIELD(sections)
SE_REFLECT_END()
