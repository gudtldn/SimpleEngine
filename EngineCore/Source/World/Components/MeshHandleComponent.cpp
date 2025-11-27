#include "World/Components/MeshHandleComponent.h"
#include "Reflection/Reflect.h"

// Reflection for MeshHandleComponent
SE_BEGIN_REFLECT(MeshHandleComponent, meta::Component)
    SE_REFLECT_PROPERTY(mesh_id, meta::Edit)
SE_END_REFLECT(MeshHandleComponent)
