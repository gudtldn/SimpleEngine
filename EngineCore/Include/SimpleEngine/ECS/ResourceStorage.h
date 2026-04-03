#pragma once

#include <type_traits>
#include <utility>


namespace se
{
/**
 * 리소스 저장소의 타입 소거(type erasure)를 위한 인터페이스
 */
class IResourceStorage
{
public:
    virtual ~IResourceStorage() = default;

    /** 리소스 데이터의 Raw Pointer를 반환합니다. */
    [[nodiscard]] virtual void* GetRaw() = 0;
    [[nodiscard]] virtual const void* GetRaw() const = 0;
};

/**
 * IResourceStorage 인터페이스와 구체적 리소스 타입을 연결하는 타입별 저장소
 * @tparam T 리소스 타입
 */
template <typename T>
class ResourceStorage final : public IResourceStorage
{
public:
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    explicit ResourceStorage(Args&&... args)
        : value(std::forward<Args>(args)...)
    {
    }

    [[nodiscard]] virtual void* GetRaw() override { return &value; }
    [[nodiscard]] virtual const void* GetRaw() const override { return &value; }

    [[nodiscard]] T& Get() { return value; }
    [[nodiscard]] const T& Get() const { return value; }

private:
    T value;
};
} // namespace se
