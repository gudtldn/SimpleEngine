export module SimpleEngine.Core.Function;
import std;


constexpr size_t SBO_BUFFER_SIZE = sizeof(void*) * 3;

export template <typename Signature>
struct Function;

export template <typename ReturnType, typename... ParamsType>
struct Function<ReturnType(ParamsType...)>
{
private:
    // 내부 저장소 및 호출을 위한 인터페이스
    struct ICallable
    {
        ICallable() = default;
        virtual ~ICallable() = default;

        ICallable(const ICallable&) = default;
        ICallable& operator=(const ICallable&) = default;
        ICallable(ICallable&&) = default;
        ICallable& operator=(ICallable&&) = default;

        virtual ReturnType Invoke(ParamsType...) = 0;

        virtual ICallable* Clone() const = 0;
        virtual void CloneTo(void* dest) const = 0;
        virtual void MoveTo(void* dest) noexcept = 0;
    };

    template <typename Fn>
    struct CallableImpl final : ICallable
    {
        Fn functor;

        template <typename F>
        explicit CallableImpl(F&& f)
            : functor(std::forward<F>(f))
        {
        }

        virtual ReturnType Invoke(ParamsType... args) override
        {
            return std::invoke(functor, std::forward<ParamsType>(args)...);
        }

        virtual ICallable* Clone() const override
        {
            return new CallableImpl(functor);
        }

        virtual void CloneTo(void* dest) const override
        {
            new(dest) CallableImpl(functor);
        }

        virtual void MoveTo(void* dest) noexcept override
        {
            new(dest) CallableImpl(std::move(functor));
        }
    };

    // Function이 가지고 있는 Callable 객체
    union FunctionStorage
    {
        ICallable* Heap_Storage = nullptr;
        std::aligned_storage_t<SBO_BUFFER_SIZE> SBO_Storage;
    } Storage;

    ICallable* CallablePtr = nullptr;

public:
    Function() noexcept = default;

    Function(std::nullptr_t) noexcept
    {
    }

    ~Function()
    {
        Reset();
    }

    Function(const Function& other)
    {
        if (other.CallablePtr)
        {
            if (other.IsOnHeap())
            {
                CallablePtr = other.CallablePtr->Clone();
                Storage.Heap_Storage = CallablePtr;
            }
            else
            {
                other.CallablePtr->CloneTo(&Storage.SBO_Storage);
                CallablePtr = reinterpret_cast<ICallable*>(&Storage.SBO_Storage);
            }
        }
    }

    Function(Function&& other) noexcept
    {
        if (other.CallablePtr)
        {
            if (other.IsOnHeap())
            {
                Storage.Heap_Storage = other.Storage.Heap_Storage;
                CallablePtr = other.CallablePtr;
            }
            else
            {
                other.CallablePtr->MoveTo(&Storage.SBO_Storage);
                CallablePtr = reinterpret_cast<ICallable*>(&Storage.SBO_Storage);
            }

            other.Storage.Heap_Storage = nullptr;
            other.CallablePtr = nullptr;
        }
    }

    Function& operator=(const Function& other)
    {
        Function temp(other);
        std::swap(temp, *this);
        return *this;
    }

    Function& operator=(Function&& other) noexcept
    {
        Reset();
        if (other.CallablePtr)
        {
            if (other.IsOnHeap())
            {
                Storage.Heap_Storage = other.Storage.Heap_Storage;
                CallablePtr = other.CallablePtr;
            }
            else
            {
                other.CallablePtr->MoveTo(&Storage.SBO_Storage);
                CallablePtr = reinterpret_cast<ICallable*>(&Storage.SBO_Storage);
            }

            other.Storage.Heap_Storage = nullptr;
            other.CallablePtr = nullptr;
        }
        return *this;
    }

    // 람다 및 기타 Callable한 객체를 받는 생성자
    template <typename Fn>
        requires (
            !std::same_as<std::decay_t<Fn>, Function>                // 자기 자신은 제외
            && !std::is_null_pointer_v<std::decay_t<Fn>>             // nullptr_t는 별도 생성자에서 처리
            && std::is_invocable_r_v<ReturnType, Fn&, ParamsType...> // 호출 가능성 검사 (반환 타입 포함)
        )
    Function(Fn&& in_func)
    {
        using DecayedFunctorType = std::decay_t<Fn>;
        using Callable = CallableImpl<DecayedFunctorType>;

        // SBO 조건: 객체 크기가 버퍼보다 작고, 이동 생성이 noexcept여야 함
        if constexpr (sizeof(Callable) <= SBO_BUFFER_SIZE && std::is_nothrow_move_constructible_v<Fn>)
        {
            new(&Storage.SBO_Storage) Callable(std::forward<Fn>(in_func));
            CallablePtr = reinterpret_cast<ICallable*>(&Storage.SBO_Storage);
        }
        else
        {
            Storage.Heap_Storage = new Callable(std::forward<Fn>(in_func));
            CallablePtr = Storage.Heap_Storage;
        }
    }

    ReturnType operator()(ParamsType... args) const
    {
        if (!CallablePtr)
        {
            throw std::bad_function_call();
        }
        return CallablePtr->Invoke(std::forward<ParamsType>(args)...);
    }

    explicit operator bool() const noexcept
    {
        return CallablePtr != nullptr;
    }

private:
    bool IsOnHeap() const noexcept
    {
        return CallablePtr && CallablePtr != reinterpret_cast<const ICallable*>(&Storage.SBO_Storage);
    }

    void Reset() noexcept
    {
        if (CallablePtr)
        {
            if (IsOnHeap())
            {
                delete CallablePtr;
            }
            else
            {
                std::destroy_at(CallablePtr);
            }
            CallablePtr = nullptr;
        }
    }
};
