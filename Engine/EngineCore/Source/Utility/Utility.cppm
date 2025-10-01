export module SE.Utility;

export import :FileUtils;
export import :Hash;
export import :ShaderUtils;
export import :StringUtils;
export import :TypeUtils;


export namespace se::utility
{
#if defined(DEBUG) || defined(_DEBUG)
constexpr bool IS_DEBUG_BUILD = true;
#else
constexpr bool IS_DEBUG_BUILD = false;
#endif

#if defined(NDEBUG)
constexpr bool IS_RELEASE_BUILD = true;
#else
constexpr bool IS_RELEASE_BUILD = false;
#endif


/**
 * 특정 값(size)을 지정된 정렬(AlignSize) 크기로 올림(round up)합니다.
 * @tparam AlignSize 정렬 기준이 되는 크기 (예: 16, 256). 반드시 2의 거듭제곱이어야 합니다.
 * @param size 정렬하려는 원래 크기 (바이트 단위)
 * @return size보다 크거나 같은 값 중에서 가장 작은 AlignSize의 배수를 반환합니다.
 */
template <size_t AlignSize>
    requires (std::has_single_bit(AlignSize))
constexpr size_t AlignedSize(size_t size)
{
    return (size + AlignSize - 1) & ~(AlignSize - 1);
}

/**
 * 특정 타입(T)의 크기를 지정된 정렬(AlignSize) 크기로 올림(round up)합니다.
 * @tparam AlignSize 정렬 기준이 되는 크기 (예: 16, 256). **반드시 2의 거듭제곱이어야 합니다.**
 * @tparam T 크기를 계산할 C++ 타입
 * @return sizeof(T)보다 크거나 같은 값 중에서 가장 작은 AlignSize의 배수를 반환합니다.
 */
template <size_t AlignSize, typename T>
    requires (std::has_single_bit(AlignSize))
constexpr size_t AlignedSize()
{
    return AlignedSize<AlignSize>(sizeof(T));
}


/**
 * 주어진 스코프({ ... })의 시작과 끝에서 특정 동작을 자동으로 수행하는 RAII 래퍼
 *
 * 객체가 생성될 때 템플릿 인자로 주어진 타입 `T`의 `Enter()` 정적 함수를 호출하고,
 * 객체가 소멸될 때(스코프를 벗어날 때) `T`의 `Exit()` 정적 함수를 호출합니다.
 *
 * @tparam T static void Enter()와 static void Exit()가 구현되어 있는 클래스
 */
template <typename T>
class ScopeGuard
{
public:
    template <typename... Args>
        requires requires { T::Enter(std::declval<Args>()...); T::Exit(); }
    ScopeGuard(Args&&... args)
    {
        T::Enter(std::forward<Args>(args)...);
    }

    ~ScopeGuard()
    {
        T::Exit();
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(const ScopeGuard&&) = delete;
    ScopeGuard& operator=(const ScopeGuard&&) = delete;
};

/**
 * 스코프를 벗어날 때 주어진 람다(lambda)나 함수 객체를 실행하는 RAII 래퍼
 * @tparam Fn 호출 가능한(invocable) 타입
 */
template <typename Fn>
    requires std::invocable<Fn>
class LambdaScopeGuard
{
public:
    LambdaScopeGuard(Fn&& in_exit_func)
        : exit_func(std::forward<Fn>(in_exit_func))
    {
    }

    ~LambdaScopeGuard()
    {
        exit_func();
    }

private:
    Fn exit_func;
};
}
