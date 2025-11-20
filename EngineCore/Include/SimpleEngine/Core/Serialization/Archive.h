#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/StringName.h"

#define SE_NVP(value) se::core::NamedValue{ value, #value }
#define SE_NVP_NAMED(value, name) se::core::NamedValue{ value, name }


namespace se::core
{
/**
 *
 */
enum class EArchiveMode
{
    Load,
    Save
};

/**
 *
 * @tparam T
 */
template <typename T>
struct NamedValue
{
    T& value;
    const char* name;
};

/**
 *
 */
class SE_CORE_API Archive
{
public:
    virtual ~Archive() = default;

    Archive(const Archive&) = delete;
    Archive& operator=(const Archive&) = delete;
    Archive(Archive&&) = delete;
    Archive& operator=(Archive&&) = delete;

public:
    [[nodiscard]] bool IsLoading() const { return mode == EArchiveMode::Load; }
    [[nodiscard]] bool IsSaving() const { return mode == EArchiveMode::Save; }

    virtual void Process(int8& value, const char* name);
    virtual void Process(uint8& value, const char* name);
    virtual void Process(int16& value, const char* name);
    virtual void Process(uint16& value, const char* name);
    virtual void Process(int32& value, const char* name);
    virtual void Process(uint32& value, const char* name);
    virtual void Process(int64& value, const char* name);
    virtual void Process(uint64& value, const char* name);
    virtual void Process(float& value, const char* name);
    virtual void Process(double& value, const char* name);
    virtual void Process(bool& value, const char* name);

    virtual void Process(String& value, const char* name);
    virtual void Process(StringName& value, const char* name);
    virtual void Process(Guid& value, const char* name);

    virtual void ProcessRaw(void* value, usize byte_size, const char* name) = 0;

    template <typename T>
    void ProcessRaw(T& value, const char* name) { ProcessRaw(&value, sizeof(T), name); }

    virtual void EnterNode(const char* name) {}
    virtual void ExitNode() {}

public:
    template <typename T>
        requires std::is_arithmetic_v<T>
    friend Archive& operator<<(Archive& ar, T& value)
    {
        ar.Process(value, nullptr);
        return ar;
    }

    template <typename T>
    friend Archive& operator<<(Archive& ar, NamedValue<T> nvp)
    {
        if constexpr (requires { ar.Process(nvp.value, nvp.name); })
        {
            ar.Process(nvp.value, nvp.name);
        }
        else
        {
            ar.EnterNode(nvp.name); // 노드 진입
            ar << nvp.value;        // 사용자 정의 operator<< 호출
            ar.ExitNode();          // 노드 탈출
        }
        return ar;
    }

    friend Archive& operator<<(Archive& ar, String& value)
    {
        ar.Process(value, nullptr);
        return ar;
    }

    friend Archive& operator<<(Archive& ar, StringName& value)
    {
        ar.Process(value, nullptr);
        return ar;
    }

    friend Archive& operator<<(Archive& ar, Guid& value)
    {
        ar.Process(value, nullptr);
        return ar;
    }

protected:
    explicit Archive(EArchiveMode in_mode) : mode(in_mode) {}
    EArchiveMode mode;
};
}
