#pragma once
#include "SimpleEngine/Asset/ImportSettings/ImportSettings.h"
#include "SimpleEngine/Reflection/Annotations.h"


namespace se::asset
{
/**
 * 3D Model 파일(FBX, OBJ, GLTF 등)을 임포트할 때 적용할 Mesh 처리 옵션
 */
class SE_CORE_API SE_TYPE_ANNOTATION() MeshImportSettings : public ImportSettings<MeshImportSettings>
{
public:
    /**
     * 소스 파일 내의 모든 서브 메쉬(Sub-mesh)를 하나의 거대한 StaticMesh로 병합할지 여부
     * - true: 모든 메쉬를 합쳐 드로우 콜을 최소화합니다. (배경, 건물, 바위 등 정적 오브젝트에 권장)
     * - false: 계층 구조(Hierarchy)를 유지하며 개별 노드로 분리합니다.
     */
    SE_PROPERTY(=meta::Edit, =meta::DisplayName("Combine Meshes"))
    bool combine_meshes = true;

    /** 노드의 트랜스폼(위치, 회전, 크기)을 버텍스 데이터에 영구적으로 적용(Bake)할지 여부 */
    SE_PROPERTY(=meta::Edit, =meta::DisplayName("Apply Transform"))
    bool apply_transform = true;

    /** 메쉬 전체에 적용할 크기 배율 */
    SE_PROPERTY(=meta::Edit, =meta::Range(0.01f, 1000.0f), =meta::DisplayName("Global Scale"))
    float global_scale = 1.0f;

public:
    virtual void Serialize(core::Archive& ar) override;
};
}  // namespace se::asset
