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


namespace se::asset
{
/**
 * Asset Import 실패 시 발생하는 에러
 */
class SE_CORE_API ImportError final : public IError
{
public:
    enum class ECode
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
    {}

    [[nodiscard]] virtual const char* What() const noexcept override
    {
        return message.CStr();
    }

    [[nodiscard]] virtual const IError* Source() const noexcept override
    {
        return source_error.get();
    }

    [[nodiscard]] ECode GetCode() const noexcept { return code; }
    [[nodiscard]] const Path& GetFilePath() const noexcept { return file_path; }

    void SetSource(std::unique_ptr<IError> source)
    {
        source_error = std::move(source);
    }

private:
    ECode code;
    String message;
    Path file_path;
    std::unique_ptr<IError> source_error;
};

/**
 * Asset Import 결과를 담는 구조체
 */
struct SE_CORE_API ImportResult
{
    /** 생성된 모든 Asset 목록 */
    Array<std::shared_ptr<IAsset>> assets;

    /** Sub-Asset 이름 -> 인덱스 매핑 */
    HashMap<String, uint32> name_to_index;

    /** 메인 Asset 인덱스 (보통 0, 없으면 UINT32_MAX) */
    uint32 main_asset_index = 0;

public:
    /** Asset이 존재하는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const
    {
        return assets.IsEmpty();
    }

    /** 생성된 Asset의 개수를 반환합니다. */
    [[nodiscard]] uint32 GetCount() const
    {
        return static_cast<uint32>(assets.Len());
    }

    /** 메인 Asset을 반환합니다. (없으면 nullptr) */
    [[nodiscard]] std::shared_ptr<IAsset> GetMainAsset() const
    {
        if (main_asset_index < assets.Len())
        {
            return assets[main_asset_index];
        }
        return nullptr;
    }

    /** 메인 Asset을 특정 타입으로 반환합니다. */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] std::shared_ptr<T> GetMainAsset() const
    {
        if (const auto main_asset = GetMainAsset())
        {
            return std::static_pointer_cast<T>(main_asset);
        }
        return nullptr;
    }

    /** 이름으로 Sub-Asset을 조회합니다. (없으면 nullptr) */
    [[nodiscard]] std::shared_ptr<IAsset> FindByName(StringView name) const
    {
        if (const auto index_opt = name_to_index.Find(name))
        {
            if (*index_opt < assets.Len())
            {
                return assets[*index_opt];
            }
        }
        return nullptr;
    }

    /** 이름으로 특정 타입의 Sub-Asset을 조회합니다. */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] std::shared_ptr<T> FindByName(StringView name) const
    {
        if (const auto asset = FindByName(name))
        {
            return std::static_pointer_cast<T>(asset);
        }
        return nullptr;
    }

    /** 인덱스로 Asset을 조회합니다. */
    [[nodiscard]] std::shared_ptr<IAsset> GetAsset(uint32 index) const
    {
        if (index < assets.Len())
        {
            return assets[index];
        }
        return nullptr;
    }

    /** 인덱스로 특정 타입의 Asset을 조회합니다. */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] std::shared_ptr<T> GetAsset(uint32 index) const
    {
        if (const auto asset = GetAsset(index))
        {
            return std::static_pointer_cast<T>(asset);
        }
        return nullptr;
    }

    /** 특정 타입의 모든 Asset을 반환합니다. */
    template <typename T>
        requires std::derived_from<T, IAsset>
    [[nodiscard]] Array<std::shared_ptr<T>> GetAllOfType() const
    {
        Array<std::shared_ptr<T>> result;
        const TypeId target_type = TypeId::Get<T>();

        for (const auto& asset : assets)
        {
            if (asset && asset->GetAssetType() == target_type)
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
            if (asset && asset->GetAssetType() == target_type)
            {
                return std::static_pointer_cast<T>(asset);
            }
        }
        return nullptr;
    }

    /** 새로 만들어진 모든 에셋의 이름 목록을 반환합니다. */
    [[nodiscard]] Array<StringView> GetAllNames() const
    {
        Array<StringView> names;
        names.Reserve(name_to_index.Len());

        for (const String& name : name_to_index | std::views::keys)
        {
            names.Push(name);
        }
        return names;
    }

public:
    /**
     * Sub-Asset을 등록합니다. (Importer 내부용)
     * @param asset 등록할 Asset
     * @param name Sub-Asset 이름 (중복 시 자동으로 suffix 추가: Name_1, Name_2, ...)
     */
    void RegisterAsset(std::shared_ptr<IAsset> asset, StringView name = {})
    {
        const uint32 index = static_cast<uint32>(assets.Len());
        assets.Push(std::move(asset));

        if (!name.IsEmpty())
        {
            String unique_name = MakeUniqueName(name);
            name_to_index.Insert(std::move(unique_name), index);
        }
    }

private:
    /** 중복되지 않는 고유한 이름을 생성합니다. */
    [[nodiscard]] String MakeUniqueName(StringView base_name)
    {
        String candidate{ base_name };

        // 이름이 이미 존재하지 않으면 그대로 반환
        if (!name_to_index.Contains(candidate))
        {
            return candidate;
        }

        // 중복 시 suffix 추가: Name_1, Name_2, ...
        uint32& next_suffix = next_suffix_map.Entry(candidate).OrInsert(1);
        do
        {
            candidate = String::Format("{}_{}", base_name, next_suffix++);
        }
        while (name_to_index.Contains(candidate));

        return candidate;
    }

    HashMap<String, uint32> next_suffix_map;
};
}  // namespace se::asset
