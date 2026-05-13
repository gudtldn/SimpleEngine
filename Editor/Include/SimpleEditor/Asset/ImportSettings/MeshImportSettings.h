#pragma once

#include "SimpleEditor/Asset/ImportSettings/ImportSettingsBase.h"

#include "SimpleEngine/Core/Reflection/Reflect.h"


namespace se::editor
{
/**
 * 3D Model 파일(FBX, OBJ, GLTF 등)을 임포트할 때 적용할 Mesh 처리 옵션
 */
class SE_EDITOR_API SE_ANNOTATION(meta::Reflect) MeshImportSettings : public ImportSettingsBase
{
    SE_CLASS(MeshImportSettings, ImportSettingsBase)

public:
    /**
     * 소스 파일 내의 모든 서브 메쉬(Sub-mesh)를 하나의 StaticMesh 에셋으로 병합할지 여부
     * - true:  모든 primitive를 1개의 에셋으로 합쳐 드로우 콜을 최소화합니다. (배경, 건물, 바위 등 거의 움직이지 않는 정적 오브젝트에 권장)
     *          제약: 첫 번째 primitive의 material만 사용됩니다. 여러 섹션에 다른 material이 필요하면 false를 사용하세요.
     * - false: 각 primitive를 독립된 에셋으로 분리합니다. 개별 섹션에 다른 머티리얼을 적용하거나, ECS에서 섹션 단위로 제어해야 할 때 사용합니다.
     */
    SE_ANNOTATION(=meta::Property, =meta::DisplayName<"Combine Meshes">{})
    bool combine_meshes = true;

    /** 노드의 트랜스폼(위치, 회전, 크기)을 버텍스 데이터에 영구적으로 적용(Bake)할지 여부 */
    SE_ANNOTATION(=meta::Property, =meta::DisplayName<"Apply Transform">{})
    bool apply_transform = true;

    /** 메쉬 전체에 적용할 크기 배율 */
    SE_ANNOTATION(=meta::Property, =meta::Range(0.01f, 1000.0f), =meta::DisplayName<"Global Scale">{})
    f32 global_scale = 1.0f;
};
} // namespace se::editor
