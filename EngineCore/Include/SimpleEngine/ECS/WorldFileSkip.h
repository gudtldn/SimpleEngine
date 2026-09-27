#pragma once


namespace se
{
/**
 * 월드 파일에 컴포넌트로 쓰지 않는 타입에 붙이는 타입 어노테이션입니다. 레거시 리플렉션의 meta::Transient에 대응합니다.
 * 예: SE_REFLECT_BEGIN(se::GlobalTransformComponent, se::WorldFileSkip{})
 */
struct WorldFileSkip
{
};
} // namespace se
