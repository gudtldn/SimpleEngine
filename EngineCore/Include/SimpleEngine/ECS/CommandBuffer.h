#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Functional/Function.h"


namespace se
{
// forward declaration
class World;

/**
 * ECS 구조 변경 명령(엔티티 생성/삭제, 컴포넌트 추가/제거 등)을 지연 실행하기 위한 버퍼
 *
 * System 실행 중에는 명령이 큐에 쌓이고, 실행이 끝난 후 Flush()를 통해 일괄 적용됩니다.
 * 이를 통해 이터레이터 무효화 및 구조 변경 충돌을 방지합니다.
 */
class SE_CORE_API CommandBuffer
{
public:
    using Command = Function<void(World&)>;

    /** 큐에 쌓인 모든 명령을 World에 순차 적용한 뒤 버퍼를 비웁니다. */
    void Flush(World& world);

    /** 버퍼가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const { return commands.IsEmpty(); }

    /** 커맨드를 큐에 추가합니다. */
    void Push(Command&& command) { commands.Push(std::move(command)); }

private:
    Array<Command> commands;
};
} // namespace se
