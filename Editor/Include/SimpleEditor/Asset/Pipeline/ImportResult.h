#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Asset/AssetId.h"
#include "SimpleEngine/Asset/AssetMetadata.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/Types/Path.h"

#include <concepts>
#include <memory>


namespace se::editor
{
/**
 * Import 실패 시 반환하는 에러
 */
class SE_EDITOR_API ImportError final : public IError
{
public:
    enum class EType : uint8
    {
        NoTranslator,      // 적합한 Translator를 찾지 못함
        TranslateFailed,   // Translator 실행 중 에러
        FactoryFailed,     // Factory에서 Asset 생성 실패
        FileNotFound,      // 파일을 찾을 수 없음
        InvalidFormat,     // 지원하지 않는 파일 형식
        CyclicDependency,  // 순환 참조 발견
        Unknown,           // 알 수 없는 에러
    };
    using enum EType;

public:
    ImportError(EType type, String message, Path file_path = {})
        : type(type)
        , message(std::move(message))
        , file_path(std::move(file_path))
    {
    }

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return source_error.get(); }
    [[nodiscard]] EType GetType() const noexcept { return type; }
    [[nodiscard]] const Path& GetFilePath() const noexcept { return file_path; }

    void SetSource(std::unique_ptr<IError> source) { source_error = std::move(source); }

private:
    EType type;
    String message;
    Path file_path;
    std::unique_ptr<IError> source_error;
};

/**
 * Import된 단일 에셋 데이터
 */
struct SE_EDITOR_API ImportedAsset
{
    String name;
    AssetId asset_id;
    std::shared_ptr<AssetBase> asset;
    Array<AssetDependencyEntry> dependencies;
};

/**
 * Asset Import 결과를 담는 불변(Immutable) 클래스
 */
class SE_EDITOR_API ImportResult final
{
public:
    class Builder;

public:
    ImportResult() = default;

    // 복사만 금지
    ImportResult(const ImportResult&) = delete;
    ImportResult& operator=(const ImportResult&) = delete;
    ImportResult(ImportResult&&) noexcept = default;
    ImportResult& operator=(ImportResult&&) noexcept = default;

public:
    /** 생성된 Asset이 존재하는지 확인합니다. */
    [[nodiscard]] FORCE_INLINE bool IsEmpty() const { return entries.IsEmpty(); }

    /** 생성된 Asset의 개수를 반환합니다. */
    [[nodiscard]] FORCE_INLINE uint32 GetCount() const { return static_cast<uint32>(entries.Len()); }

    /** Entry 목록의 참조를 반환합니다. */
    [[nodiscard]] FORCE_INLINE const Array<ImportedAsset>& GetEntries() const { return entries; }

    /** 메인 에셋 Entry를 반환합니다. 등록된 Entry가 없으면 NullOpt. */
    [[nodiscard]] Optional<const ImportedAsset&> GetMainAsset() const;

    /**
     * 이름으로 에셋 Entry를 검색합니다.
     * @param name 검색할 에셋 이름
     * @return 일치하는 Entry, 없으면 NullOpt
     */
    [[nodiscard]] Optional<const ImportedAsset&> FindByName(StringView name) const;

    /**
     * 이름으로 타입이 지정된 에셋 포인터를 반환합니다.
     * @tparam T 캐스팅할 에셋 타입 (AssetBase 파생 타입)
     * @param name 검색할 에셋 이름
     * @return 캐스팅 성공 시 shared_ptr<T>, 없거나 타입 불일치 시 nullptr
     */
    template <typename T>
        requires std::derived_from<T, AssetBase>
    [[nodiscard]] std::shared_ptr<T> GetAsset(StringView name) const
    {
        if (const auto entry = FindByName(name))
        {
            return std::dynamic_pointer_cast<T>(entry->asset);
        }
        return nullptr;
    }

private:
    ImportResult(
        Array<ImportedAsset> in_entries,
        HashMap<String, uint32> in_name_to_index,
        uint32 in_main_asset_index
    );

private:
    /** 생성된 모든 Asset 목록 */
    Array<ImportedAsset> entries;

    /** Sub-Asset 이름 -> 인덱스 매핑 */
    HashMap<String, uint32> name_to_index;

    /** 메인 Asset 인덱스 */
    uint32 main_asset_index = 0;
};

/**
 * ImportResult 생성을 담당하는 Builder
 */
class SE_EDITOR_API ImportResult::Builder
{
public:
    /**
     * Asset을 등록하고 인덱스를 반환합니다.
     * @param name 식별 이름 (중복 시 자동 변경됨)
     * @param asset_id 해당 에셋의 고유 ID
     * @param asset 등록할 에셋 인스턴스
     * @param dependencies 이 에셋이 의존하는 에셋 목록
     * @return 등록된 엔트리의 인덱스
     */
    uint32 RegisterAsset(
        const String& name,
        AssetId asset_id,
        std::shared_ptr<AssetBase> asset,
        Array<AssetDependencyEntry> dependencies = {}
    );

    /** 메인 에셋의 인덱스를 설정합니다. */
    void SetMainAssetIndex(uint32 index);

    /** 최종 결과 객체를 생성합니다. 호출 후 빌더는 초기화됩니다. */
    [[nodiscard]] ImportResult Build();

private:
    /** 중복되지 않는 고유한 이름을 생성합니다. */
    [[nodiscard]] String MakeUniqueName(const String& base_name);

private:
    Array<ImportedAsset> entries;
    HashMap<String, uint32> name_to_index;
    HashMap<String, uint32> next_suffix_map; // 임시 상태 저장용
    uint32 main_asset_index = 0;
};
} // namespace se::editor
