#pragma once
#include <compare>

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


/**
 * 128비트 전역 고유 식별자(GUID/UUID) 클래스
 */
class SE_CORE_API Guid
{
public:
    Guid() noexcept = default;
    ~Guid() = default;

    Guid(const Guid& other) noexcept = default;
    Guid& operator=(const Guid& other) noexcept = default;
    Guid(Guid&& other) noexcept = default;
    Guid& operator=(Guid&& other) noexcept = default;

public:
    static const Guid None;

    [[nodiscard]] static Guid NewGuid();
    [[nodiscard]] static Guid FromString(const se::String& str);

public:
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] se::String ToString() const;

public:
    [[nodiscard]] explicit operator bool() const noexcept;
    [[nodiscard]] bool operator==(const Guid& other) const noexcept = default;

private:
    se::FixedArray<uint8, 16> data{};
};

template <>
struct SE_CORE_API std::hash<Guid>
{
    size_t operator()(const Guid& guid) const noexcept;
};
