#pragma once

#include "SimpleEngine/Traits/TypeTraits.h"
#include "SimpleEngine/Utility/Debug.h"

#include <type_traits>


namespace se
{
/**
 * ECS 시스템 내에서 전역 리소스의 참조와 접근 권한을 명시하는 타입
 * @tparam T 리소스 접근 타입 (const T& = 읽기 전용, T& = 쓰기 가능)
 */
template <typename T>
class Resource
{
    static_assert(std::is_reference_v<T>,
        "Resource<T> requires T to be a reference type. Use Resource<const T&> or Resource<T&>.");

public:
    using RawType = std::remove_cvref_t<T>;
    using RefType = traits::CopyConst<T, RawType>;

    static constexpr bool IS_READ_ONLY = std::is_const_v<RefType>;

public:
    explicit Resource(RefType& in_ref)
        : ref(std::addressof(in_ref))
    {
    }

    [[nodiscard]] RefType& operator*() const
    {
        SE_ASSERT(ref, "Resource is null.");
        return *ref;
    }

    [[nodiscard]] RefType* operator->() const
    {
        SE_ASSERT(ref, "Resource is null.");
        return ref;
    }

    [[nodiscard]] bool IsValid() const { return ref != nullptr; }
    [[nodiscard]] explicit operator bool() const { return IsValid(); }

private:
    RefType* ref;
};
} // namespace se
