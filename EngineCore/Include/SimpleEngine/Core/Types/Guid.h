#pragma once
#include "SimpleEngine/Core/Container/FixedArray.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"


namespace se
{
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
    [[nodiscard]] static Guid FromString(const String& str);

public:
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] String ToString() const;

public:
    [[nodiscard]] explicit operator bool() const noexcept;
    [[nodiscard]] bool operator==(const Guid& other) const noexcept = default;

private:
    FixedArray<uint8, 16> data{};
};
}  // namespace se

template <>
struct SE_CORE_API std::hash<se::Guid>
{
    size_t operator()(const se::Guid& guid) const noexcept;
};
