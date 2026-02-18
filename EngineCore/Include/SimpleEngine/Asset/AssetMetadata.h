#pragma once
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Reflection/Annotations.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Types/Guid.h"


namespace se::asset
{
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

    /** 소스 파일의 SHA-256 해시 (변경 감지용, "sha256:..." 형태) */
    SE_ANNOTATION(=meta::Property)
    String source_hash;

    /** 사용할 Importer/Translator의 타입 식별자 */
    SE_ANNOTATION(=meta::Property)
    TypeId importer_type;

    /** 캐시 바이너리의 스키마 버전 (Importer 출력 포맷 변경 시 증가) */
    SE_ANNOTATION(=meta::Property)
    uint32 cache_version = 0;

    /** 이 소스 파일에서 생성된 Sub-Asset 목록 */
    SE_ANNOTATION(=meta::Property)
    Array<SubAssetMeta> sub_assets;

    bool operator==(const AssetMetadata&) const = default;
};
} // namespace se::asset

SE_DECLARE_REFLECTION(se::asset::SubAssetMeta)
SE_DECLARE_REFLECTION(se::asset::AssetMetadata)
