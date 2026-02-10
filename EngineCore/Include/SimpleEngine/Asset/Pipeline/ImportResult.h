#pragma once
#include <concepts>
#include <memory>

#include "SimpleEngine/Asset/Types/IAsset.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Utility/Debug.h"


namespace se::asset
{
/**
 * Asset Import 실패 시 발생하는 에러
 */
class SE_CORE_API ImportError final : public IError
{
public:
    enum class ECode : uint8
    {
        NoTranslator,      // 적합한 Translator를 찾지 못함
        TranslateFailed,   // Translator 실행 중 에러
        FactoryFailed,     // Factory에서 Asset 생성 실패
        FileNotFound,      // 파일을 찾을 수 없음
        InvalidFormat,     // 지원하지 않는 파일 형식
        CyclicDependency,  // 순환 참조 발견
        Unknown,           // 알 수 없는 에러
    };

public:
    ImportError(ECode code, String message, Path file_path = {})
        : code(code)
        , message(std::move(message))
        , file_path(std::move(file_path))
    {
    }

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return source_error.get(); }

    [[nodiscard]] ECode GetCode() const noexcept { return code; }
    [[nodiscard]] const Path& GetFilePath() const noexcept { return file_path; }

    void SetSource(std::unique_ptr<IError> source) { source_error = std::move(source); }

private:
    ECode code;
    String message;
    Path file_path;
    std::unique_ptr<IError> source_error;
};

/**
 * Asset Import 결과를 담는 불변(Immutable) 클래스
 */
class SE_CORE_API ImportResult final
{
public:
    class Builder; // Forward Declaration

    using IteratorType = Array<std::shared_ptr<IAsset>>::IteratorType;
    using ConstIteratorType = Array<std::shared_ptr<IAsset>>::ConstIteratorType;

public:
    ImportResult() = default;

    // 복사만 금지
    ImportResult(const ImportResult&) = delete;
    ImportResult& operator=(const ImportResult&) = delete;
    ImportResult(ImportResult&&) noexcept = default;
    ImportResult& operator=(ImportResult&&) noexcept = default;

public:
    [[nodiscard]] FORCE_INLINE const Array<std::shared_ptr<IAsset>>& GetAssets() const { return assets; }
    [[nodiscard]] FORCE_INLINE const HashMap<String, uint32>& GetNameToIndexMap() const { return name_to_index; }

    /** Asset이 존재하는지 확인합니다. */
    [[nodiscard]] FORCE_INLINE bool IsEmpty() const { return assets.IsEmpty(); }

    /** 생성된 Asset의 개수를 반환합니다. */
    [[nodiscard]] FORCE_INLINE uint32 GetCount() const { return static_cast<uint32>(assets.Len()); }

    /** 메인 Asset을 반환합니다. (없으면 nullptr) */
    [[nodiscard]] std::shared_ptr<IAsset> GetMainAsset() const;

    /** 인덱스로 Asset을 조회합니다. */
    [[nodiscard]] std::shared_ptr<IAsset> GetAsset(uint32 index) const;

    /** 이름으로 Sub-Asset을 조회합니다. (없으면 nullptr) */
    [[nodiscard]] std::shared_ptr<IAsset> FindByName(StringView name) const;

    /** 새로 만들어진 모든 에셋의 이름 목록을 반환합니다. */
    [[nodiscard]] Array<StringView> GetAllNames() const;

public:
    /** 메인 Asset을 특정 타입으로 반환합니다. */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] std::shared_ptr<T> GetMainAsset() const
    {
        return CastAsset<T>(GetMainAsset());
    }

    /** 이름으로 특정 타입의 Sub-Asset을 조회합니다. */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] std::shared_ptr<T> FindByName(StringView name) const
    {
        return CastAsset<T>(FindByName(name));
    }

    /** 인덱스로 특정 타입의 Asset을 조회합니다. */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] std::shared_ptr<T> GetAsset(uint32 index) const
    {
        return CastAsset<T>(GetAsset(index));
    }

    /** 특정 타입의 모든 Asset을 반환합니다. */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] Array<std::shared_ptr<T>> GetAllOfType() const
    {
        Array<std::shared_ptr<T>> result;
        const TypeId target_type = TypeId::Get<T>();

        result.Reserve(assets.Len());
        for (const auto& asset : assets)
        {
            if (asset && asset->GetTypeId() == target_type)
            {
                result.Push(std::static_pointer_cast<T>(asset));
            }
        }
        return result;
    }

    /** 첫 번째로 발견되는 특정 타입의 Asset을 반환합니다. */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] std::shared_ptr<T> GetFirstOfType() const
    {
        const TypeId target_type = TypeId::Get<T>();

        for (const auto& asset : assets)
        {
            if (asset && asset->GetTypeId() == target_type)
            {
                return std::static_pointer_cast<T>(asset);
            }
        }
        return nullptr;
    }

public:
    [[nodiscard]] IteratorType begin() { return assets.begin(); }
    [[nodiscard]] IteratorType end() { return assets.end(); }
    [[nodiscard]] ConstIteratorType begin() const { return assets.begin(); }
    [[nodiscard]] ConstIteratorType end() const { return assets.end(); }

private:
    ImportResult(
        Array<std::shared_ptr<IAsset>> assets,
        HashMap<String, uint32> name_to_index,
        uint32 main_asset_index
    );

    template <typename T>
    [[nodiscard]] std::shared_ptr<T> CastAsset(const std::shared_ptr<IAsset>& asset) const
    {
        if (asset)
        {
            const TypeId target_type = TypeId::Get<T>();
            if (SE_ENSURE(asset->GetTypeId() == target_type, "Asset type mismatch! Asset: {}, Requested: {}", asset->GetTypeId().GetName(), target_type.GetName()))
            {
                return std::static_pointer_cast<T>(asset);
            }
        }
        return nullptr;
    }

private:
    /** 생성된 모든 Asset 목록 */
    Array<std::shared_ptr<IAsset>> assets;

    /** Sub-Asset 이름 -> 인덱스 매핑 */
    HashMap<String, uint32> name_to_index;

    /** 메인 Asset 인덱스 */
    uint32 main_asset_index = 0;
};

/**
 * ImportResult 생성을 담당하는 Builder 클래스
 */
class SE_CORE_API ImportResult::Builder
{
public:
    Builder() = default;

    /**
     * Asset을 등록하고 인덱스를 반환합니다.
     * @param asset 등록할 Asset
     * @param name 식별 이름 (중복 시 자동 변경됨)
     */
    uint32 RegisterAsset(std::shared_ptr<IAsset> asset, const String& name = {});

    /** 메인 에셋의 인덱스를 설정합니다. */
    void SetMainAssetIndex(uint32 index);

    /** 최종 결과 객체를 생성합니다. 호출 후 빌더는 초기화됩니다. */
    [[nodiscard]] ImportResult Build();

private:
    /** 중복되지 않는 고유한 이름을 생성합니다. */
    [[nodiscard]] String MakeUniqueName(const String& base_name);

private:
    Array<std::shared_ptr<IAsset>> assets;
    HashMap<String, uint32> name_to_index;
    HashMap<String, uint32> next_suffix_map; // 임시 상태 저장용
    uint32 main_asset_index = 0;
};

}  // namespace se::asset
