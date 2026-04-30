#pragma once

#include "SimpleEngine/Asset/HandleTable.h"
#include "SimpleEngine/Asset/Types/AssetBase.h"

#include <concepts>
#include <utility>


namespace se
{
/**
 * SlotEntry를 가리키는 Generational Handle 클래스
 *
 * 내부적으로 { index, generation, table* }을 관리하며,
 * ref_count 기반의 참조 카운팅을 통해 에셋의 수명을 제어합니다.
 *
 * @tparam T 핸들이 참조하는 Asset의 타입 (ex: Texture, Material, Mesh)
 */
template <typename T>
    requires std::derived_from<T, AssetBase>
class AssetHandle
{
public:
    using AssetType = T;
    static constexpr uint32 INVALID_INDEX = HandleData::INVALID_INDEX;
    static constexpr uint32 INVALID_GENERATION = HandleData::INVALID_GENERATION;

public:
    AssetHandle() = default;

    AssetHandle(HandleData handle_data, HandleTable* in_table)
        : index(handle_data.index)
        , generation(handle_data.generation)
        , table(in_table)
    {
        if (table && index != INVALID_INDEX)
        {
            table->GetSlot(index).ref_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    ~AssetHandle()
    {
        Release();
    }

    AssetHandle(const AssetHandle& other)
        : index(other.index)
        , generation(other.generation)
        , table(other.table)
    {
        if (table && index != INVALID_INDEX)
        {
            table->GetSlot(index).ref_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    AssetHandle& operator=(const AssetHandle& other)
    {
        if (this != &other)
        {
            Release();
            index = other.index;
            generation = other.generation;
            table = other.table;
            if (table && index != INVALID_INDEX)
            {
                table->GetSlot(index).ref_count.fetch_add(1, std::memory_order_relaxed);
            }
        }
        return *this;
    }

    AssetHandle(AssetHandle&& other) noexcept
        : index(std::exchange(other.index, INVALID_INDEX))
        , generation(std::exchange(other.generation, INVALID_GENERATION))
        , table(std::exchange(other.table, nullptr))
    {
    }

    AssetHandle& operator=(AssetHandle&& other) noexcept
    {
        if (this != &other)
        {
            Release();
            index = std::exchange(other.index, INVALID_INDEX);
            generation = std::exchange(other.generation, INVALID_GENERATION);
            table = std::exchange(other.table, nullptr);
        }
        return *this;
    }

public:
    /**
     * 에셋의 Raw Pointer를 반환합니다. (High Performance)
     * @return 유효한 에셋 포인터 또는 nullptr
     */
    [[nodiscard]] T* Get() const
    {
        if (!table || index == INVALID_INDEX)
        {
            return nullptr;
        }

        const SlotEntry& entry = table->GetSlot(index);
        if (entry.generation != generation)
        {
            return nullptr; // stale handle
        }

        SE_ASSERT(entry.asset_type == TypeId::Get<AssetType>());
        return static_cast<T*>(entry.asset.load(std::memory_order_acquire));
    }

    /** 현재 핸들이 유효한 에셋을 가리키고 있는지 확인합니다. */
    [[nodiscard]] bool IsValid() const
    {
        if (!table || index == INVALID_INDEX)
        {
            return false;
        }

        const SlotEntry& entry = table->GetSlot(index);
        return entry.generation == generation
            && entry.GetState() == ELoadingState::Loaded;
    }

    /**
     * 이 핸들이 가리키는 에셋의 고유 ID를 반환합니다.
     * 핸들이 무효한 경우 AssetId::Invalid를 반환합니다.
     */
    [[nodiscard]] AssetId GetAssetId() const
    {
        if (!table || index == INVALID_INDEX)
        {
            return AssetId::Invalid;
        }

        const SlotEntry& entry = table->GetSlot(index);
        if (entry.generation != generation)
        {
            return AssetId::Invalid; // stale handle
        }
        return entry.asset_id;
    }

public:
    [[nodiscard]] T* operator->() const
    {
        return Get();
    }

    [[nodiscard]] T& operator*() const
    {
        SE_ASSERT(Get() != nullptr, "AssetHandle is invalid");
        return *Get();
    }

    [[nodiscard]] explicit operator bool() const { return IsValid(); }

    [[nodiscard]] bool operator==(const AssetHandle& other) const
    {
        return index == other.index
            && generation == other.generation
            && table == other.table;
    }

private:
    void Release()
    {
        if (table && index != INVALID_INDEX)
        {
            if (table->GetSlot(index).ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
            {
                table->MarkForEviction(index);
            }
        }
        index = INVALID_INDEX;
        generation = INVALID_GENERATION;
        table = nullptr;
    }

private:
    uint32 index = INVALID_INDEX;
    uint32 generation = INVALID_GENERATION;
    HandleTable* table = nullptr;
};
} // namespace se
