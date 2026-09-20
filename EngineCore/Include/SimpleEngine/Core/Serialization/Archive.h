#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Reflection/TypeShape.h"


namespace se
{
/** Int/Enum 노드가 다루는 정수 폭 */
enum class EIntWidth : u8
{
    Bits8,
    Bits16,
    Bits32,
    Bits64,
};

/** Float 노드가 다루는 부동소수점 폭 */
enum class EFloatWidth : u8
{
    Bits32,
    Bits64,
};

/**
 * Plan이 걸어가는 노드를 포맷별 바이트로 옮기는 추상 인터페이스
 */
class SE_CORE_API Archive
{
public:
    virtual ~Archive();

    Archive() = default;
    Archive(const Archive&) = delete;
    Archive& operator=(const Archive&) = delete;
    Archive(Archive&&) = delete;
    Archive& operator=(Archive&&) = delete;

public:
    // 값
    virtual void Int(i64& value, EIntWidth width, bool is_signed) = 0;
    virtual void Float(f64& value, EFloatWidth width) = 0;
    virtual void Bool(bool& value) = 0;
    virtual void Bytes(void* data, u64 size) = 0;
    virtual void Enum(i64& value, ArrayView<const EnumEntry> entries, EIntWidth width) = 0;

    // 구조
    virtual void BeginStruct() = 0;
    [[nodiscard]] virtual bool Field(u32 field_id, StringView name, u8 wire_type) = 0;
    virtual void EndStruct() = 0;
    virtual void BeginSeq(u64& count) = 0;
    virtual void EndSeq() = 0;
    virtual void BeginMapEntry() = 0;
    virtual void EndMapEntry() = 0;

    /** Optional 노드가 값을 가지고 있는지 여부(field presence)를 읽거나 씁니다. */
    virtual void Present(bool& has_value) = 0;

public:
    /** 에러가 발생했는지 확인합니다. 한 번 켜지면 이후 모든 연산은 no-op이어야 합니다. */
    [[nodiscard]] bool HasError() const { return has_error; }

    /** 에러 메시지를 반환합니다. */
    [[nodiscard]] StringView GetError() const { return error_message; }

    /** 에러 상태를 설정합니다. 이후 모든 연산에서 이 상태를 확인해 no-op으로 만들어야 합니다. */
    void SetError(String reason);

private:
    bool has_error = false;
    String error_message;
};
} // namespace se
