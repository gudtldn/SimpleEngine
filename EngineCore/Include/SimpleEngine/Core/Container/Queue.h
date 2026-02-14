// ReSharper disable CppRedundantTypenameKeyword
#pragma once
#include "SimpleEngine/Core/Container/Deque.h"


namespace se
{
/**
 * FIFO(First-In, First-Out) 정책을 따르는 컨테이너 어댑터
 * @detail 내부적으로 se::Deque를 사용하여 구현됩니다
 * @tparam T 요소의 타입
 * @tparam Container 내부적으로 사용할 컨테이너의 타입. (se::Deque<T>와 호환되어야 합니다.)
 */
template <typename T, typename Container = Deque<T>>
class Queue
{
public:
    using ContainerType = Container;
    using ValueType = ContainerType::ValueType;
    using SizeType = ContainerType::SizeType;

public:
    Queue() = default;
    explicit Queue(const ContainerType& cont);
    explicit Queue(ContainerType&& cont);

public:
    /** Queue가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const;

    /** Queue에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const;

    /** Queue의 모든 요소를 제거합니다. */
    void Clear() noexcept;

    /** Queue의 맨 앞에 있는 요소(가장 먼저 나갈 요소)에 대한 Optional 참조를 반환합니다. */
    [[nodiscard]] Optional<ValueType&> Front();
    [[nodiscard]] Optional<const ValueType&> Front() const;

    /** Queue의 맨 뒤에 있는 요소(가장 마지막에 들어온 요소)에 대한 Optional 참조를 반환합니다. */
    [[nodiscard]] Optional<ValueType&> Back();
    [[nodiscard]] Optional<const ValueType&> Back() const;

    /**
     * Queue의 맨 뒤에 새 요소를 추가합니다.
     * @param value 추가할 요소
     */
    void Push(const ValueType& value);
    void Push(ValueType&& value);

    /**
     * Queue의 맨 뒤에 새 요소를 내부 생성(emplace)합니다.
     * @return 생성된 요소의 참조
     */
    template <typename... Args>
    ValueType& Emplace(Args&&... args);

    /** Queue의 맨 앞에서 요소를 제거하고, 그 값을 Optional로 반환합니다. */
    Optional<ValueType> Pop();

    /** 두 Queue의 내용을 교환합니다. */
    void Swap(Queue& other) noexcept;

public:
    friend void swap(Queue& lhs, Queue& rhs) noexcept
    {
        lhs.Swap(rhs);
    }

private:
    ContainerType container;
};


template <typename T, typename Container>
Queue<T, Container>::Queue(const ContainerType& cont)
    : container(cont)
{
}

template <typename T, typename Container>
Queue<T, Container>::Queue(ContainerType&& cont)
    : container(std::move(cont))
{
}

template <typename T, typename Container>
bool Queue<T, Container>::IsEmpty() const
{
    return container.IsEmpty();
}

template <typename T, typename Container>
Queue<T, Container>::SizeType Queue<T, Container>::Len() const
{
    return container.Len();
}

template <typename T, typename Container>
void Queue<T, Container>::Clear() noexcept
{
    container.Clear();
}

template <typename T, typename Container>
Optional<typename Queue<T, Container>::ValueType&> Queue<T, Container>::Front()
{
    return container.Front();
}

template <typename T, typename Container>
Optional<const typename Queue<T, Container>::ValueType&> Queue<T, Container>::Front() const
{
    return container.Front();
}

template <typename T, typename Container>
Optional<typename Queue<T, Container>::ValueType&> Queue<T, Container>::Back()
{
    return container.Back();
}

template <typename T, typename Container>
Optional<const typename Queue<T, Container>::ValueType&> Queue<T, Container>::Back() const
{
    return container.Back();
}

template <typename T, typename Container>
void Queue<T, Container>::Push(const ValueType& value)
{
    container.PushBack(value);
}

template <typename T, typename Container>
void Queue<T, Container>::Push(ValueType&& value)
{
    container.PushBack(std::move(value));
}

template <typename T, typename Container>
template <typename... Args>
Queue<T, Container>::ValueType& Queue<T, Container>::Emplace(Args&&... args)
{
    return container.EmplaceBack(std::forward<Args>(args)...);
}

template <typename T, typename Container>
Optional<typename Queue<T, Container>::ValueType> Queue<T, Container>::Pop()
{
    return container.PopFront();
}

template <typename T, typename Container>
void Queue<T, Container>::Swap(Queue& other) noexcept
{
    container.Swap(other.container);
}
}  // namespace se
