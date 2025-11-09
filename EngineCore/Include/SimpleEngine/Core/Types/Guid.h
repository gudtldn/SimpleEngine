#pragma once
#include <compare>

#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


/**
 * @todo docs
 */
class SE_CORE_API Guid
{
public:
    Guid() noexcept;
    ~Guid();

    Guid(const Guid& other) noexcept;
    Guid& operator=(const Guid& other) noexcept;
    Guid(Guid&& other) noexcept;
    Guid& operator=(Guid&& other) noexcept;

public:
    static Guid None;

    [[nodiscard]] static Guid NewGuid();
    [[nodiscard]] static Guid FromString(const se::String& str);

public:
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] se::String ToString() const;

public:
    [[nodiscard]] explicit operator bool() const noexcept;
    [[nodiscard]] bool operator==(const Guid& other) const noexcept = default;

private:
    se::FixedArray<uint8, 16> data;
};

template <>
struct std::hash<Guid>
{
    size_t operator()(const Guid& guid) const noexcept;
};
