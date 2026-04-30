#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/HashDigest.h"


namespace se
{
/**
 * 에셋 의존성의 종류를 나타내는 열거형
 */
enum class EAssetDependencyType : uint8
{
    Hard,      // 변경 시 반드시 Re-cook + Hot-Reload
    Soft,      // 변경 시 Re-cook 권장, 무시 가능
    BuildOnly, // 빌드/쿠킹에만 필요, 런타임에 로드하지 않음
};

/**
 * .meta 파일에 기록되는 개별 의존성 항목
 *
 * Import/Cook 시 Translator가 반환한 의존 파일 목록을 이 구조체로 표현합니다.
 * .meta의 [[metadata.sub_assets.dependencies]] 섹션에 직렬화됩니다.
 */
struct SE_ANNOTATION(=meta::SerializeOnly) AssetDependencyEntry
{
    /** 의존 대상 소스 파일의 가상 경로 (예: "Assets://Textures/Wood_Diffuse.png") */
    SE_ANNOTATION(=meta::Property)
    String source_vpath;

    /** 특정 Sub-asset의 GUID (비어있으면 파일 전체에 의존) */
    SE_ANNOTATION(=meta::Property)
    Guid asset_guid;

    /** 의존성 종류 */
    SE_ANNOTATION(=meta::Property)
    EAssetDependencyType type = EAssetDependencyType::Hard;

    bool operator==(const AssetDependencyEntry&) const = default;
};

/**
 * .meta 파일 내 개별 Sub-Asset 정보를 나타내는 구조체
 *
 * 하나의 소스 파일(예: character.fbx)에서 여러 Sub-Asset이 생성될 때,
 * 각 Sub-Asset의 이름, GUID, 타입 정보를 담습니다.
 */
struct SE_ANNOTATION(=meta::SerializeOnly) SubAssetMeta
{
    /** Sub-Asset의 이름 (예: "Mesh_Character", "Material_Body") */
    SE_ANNOTATION(=meta::Property)
    String name;

    /** Sub-Asset의 고유 식별자 */
    SE_ANNOTATION(=meta::Property)
    Guid guid;

    /** Sub-Asset의 타입 식별자 */
    SE_ANNOTATION(=meta::Property)
    TypeId type;

    /** 이 Sub-Asset이 의존하는 다른 에셋 목록 */
    SE_ANNOTATION(=meta::Property)
    Array<AssetDependencyEntry> dependencies;

    bool operator==(const SubAssetMeta&) const = default;
};

/**
 * .meta 파일 전체를 표현하는 데이터 구조체 (DTO)
 *
 * Editor가 TOML .meta 파일을 파싱/생성할 때 이 구조체를 사용하고, Core의 AssetRegistry에 데이터를 주입합니다.
 * Core는 이 구조체의 존재만 알 뿐, TOML 파싱은 수행하지 않습니다.
 */
struct SE_ANNOTATION(=meta::SerializeOnly) AssetMetadata
{
    /** 소스 파일의 Primary GUID */
    SE_ANNOTATION(=meta::Property)
    Guid guid;

    /** 소스 파일의 SHA-256 해시 (변경 감지용) */
    SE_ANNOTATION(=meta::Property)
    ContentHash source_hash;

    /** 소스 파일의 마지막 수정 시간 */
    SE_ANNOTATION(=meta::Property)
    uint64 source_mtime = 0;

    /** 소스 파일의 크기 */
    SE_ANNOTATION(=meta::Property)
    uint64 source_size = 0;

    /** 캐시 바이너리의 스키마 버전 (Importer 출력 포맷 변경 시 증가) */
    SE_ANNOTATION(=meta::Property)
    uint32 cache_version = 0;

    /** Import Settings의 SHA-256 해시 (설정 변경 감지용) */
    SE_ANNOTATION(=meta::Property)
    ContentHash settings_hash;

    /** 이 소스 파일에서 생성된 Sub-Asset 목록 */
    SE_ANNOTATION(=meta::Property)
    Array<SubAssetMeta> sub_assets;

    bool operator==(const AssetMetadata&) const = default;
};
} // namespace se

SE_DECLARE_REFLECTION(se::AssetDependencyEntry)
SE_DECLARE_REFLECTION(se::SubAssetMeta)
SE_DECLARE_REFLECTION(se::AssetMetadata)
