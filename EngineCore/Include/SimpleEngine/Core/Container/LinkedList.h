#pragma once
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <memory>

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Memory/Allocators.h"


namespace se
{
// Forward declarations
template <typename T, typename Allocator>
class LinkedList;

template <typename T>
class ConstLinkedListIterator;

template <typename T>
struct LinkedListNode;

/**
 * LinkedList를 위한 비상수(non-const) 양방향 이터레이터입니다.
 */
template <typename T>
class LinkedListIterator
{
    template <typename, typename>
    friend class LinkedList;
    friend class ConstLinkedListIterator<T>;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = isize;
    using pointer = T*;
    using reference = T&;

public:
    LinkedListIterator() = default;

    reference operator*() const;
    pointer operator->() const;
    LinkedListIterator& operator++();
    LinkedListIterator operator++(int);
    LinkedListIterator& operator--();
    LinkedListIterator operator--(int);

    bool operator==(const LinkedListIterator& other) const { return node_ptr == other.node_ptr; }

private:
    using Node = LinkedListNode<T>;

    explicit LinkedListIterator(Node* in_node_ptr)
        : node_ptr(in_node_ptr)
    {
    }

private:
    Node* node_ptr = nullptr;
};

/**
 * LinkedList를 위한 상수(const) 양방향 이터레이터입니다.
 */
template <typename T>
class ConstLinkedListIterator
{
    template <typename, typename>
    friend class LinkedList;

public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = isize;
    using pointer = const T*; // const pointer
    using reference = const T&; // const reference

public:
    ConstLinkedListIterator() = default;
    ConstLinkedListIterator(const LinkedListIterator<T>& other);

    reference operator*() const;
    pointer operator->() const;
    ConstLinkedListIterator& operator++();
    ConstLinkedListIterator operator++(int);
    ConstLinkedListIterator& operator--();
    ConstLinkedListIterator operator--(int);

    bool operator==(const ConstLinkedListIterator& other) const { return node_ptr == other.node_ptr; }

private:
    using Node = LinkedListNode<T>;
    const Node* node_ptr = nullptr;

    explicit ConstLinkedListIterator(const Node* in_node_ptr) : node_ptr(in_node_ptr) {}
};

/**
 * DoubleLinkedList Container
 * @tparam T 요소의 타입
 * @tparam Allocator 노드 메모리를 위한 할당자 타입
 */
template <typename T, typename Allocator = core::DefaultAllocator<T>>
class LinkedList
{
private:
    using Node = LinkedListNode<T>;
    using NodeAllocatorType = std::allocator_traits<Allocator>::template rebind_alloc<Node>;
    using NodeAllocTraits = std::allocator_traits<NodeAllocatorType>;

public:
    using ValueType = T;
    using AllocatorType = Allocator;
    using SizeType = usize;
    using Iterator = LinkedListIterator<T>;
    using ConstIterator = ConstLinkedListIterator<T>;

public:
    LinkedList() noexcept;
    explicit LinkedList(SizeType count);
    LinkedList(SizeType count, const ValueType& value);
    LinkedList(std::initializer_list<ValueType> init_list);

    template <std::input_iterator It, std::sentinel_for<It> Sent>
        requires std::same_as<std::iter_value_t<It>, T>
    LinkedList(It first, Sent last);

    ~LinkedList();

    LinkedList(const LinkedList& other);
    LinkedList& operator=(const LinkedList& other);
    LinkedList(LinkedList&& other) noexcept;
    LinkedList& operator=(LinkedList&& other) noexcept;

public:
    template <std::ranges::input_range Rng>
        requires std::same_as<std::ranges::range_value_t<Rng>, T>
    [[nodiscard]] static LinkedList FromRange(Rng&& range);

public:
    /** 리스트에 포함된 요소의 수를 반환합니다. */
    [[nodiscard]] SizeType Len() const noexcept;

    /** 리스트가 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const noexcept;

    /** 모든 요소를 제거하여 리스트를 비웁니다. */
    void Clear();

    /** 첫 번째 요소에 대한 Optional 참조를 반환합니다. */
    [[nodiscard]] Optional<T&> Front();
    [[nodiscard]] Optional<const T&> Front() const;

    /** 마지막 요소에 대한 Optional 참조를 반환합니다. */
    [[nodiscard]] Optional<T&> Back();
    [[nodiscard]] Optional<const T&> Back() const;

    /**
     * 리스트의 맨 앞에 새 요소를 추가합니다.
     * @param value 추가할 요소
     */
    void PushFront(const ValueType& value);
    void PushFront(ValueType&& value);
    /**
     * 리스트의 맨 뒤에 새 요소를 추가합니다.
     * @param value 추가할 요소
     */
    void PushBack(const ValueType& value);
    void PushBack(ValueType&& value);

    /** 리스트의 맨 앞에서 요소를 제거하고, 그 값을 Optional로 반환합니다. */
    Optional<ValueType> PopFront();

    /** 리스트의 맨 뒤에서 요소를 제거하고, 그 값을 Optional로 반환합니다. */
    Optional<ValueType> PopBack();

    /**
     * 리스트의 맨 앞에 새 요소를 내부 생성(emplace)합니다.
     * @return 생성된 요소의 참조
     */
    template <typename... Args>
    T& EmplaceFront(Args&&... args);

    /**
     * 리스트의 맨 뒤에 새 요소를 내부 생성(emplace)합니다.
     * @return 생성된 요소의 참조
     */
    template <typename... Args>
    T& EmplaceBack(Args&&... args);

    /**
     * 지정된 이터레이터 위치 앞에 새 요소를 삽입합니다.
     * @param where 삽입할 위치를 가리키는 이터레이터
     * @param value 삽입할 요소
     * @return 삽입된 요소를 가리키는 새로운 이터레이터
     */
    Iterator Insert(Iterator where, const ValueType& value);
    Iterator Insert(Iterator where, ValueType&& value);

    /** 지정된 이터레이터 위치 앞에 새 요소를 내부 생성(emplace)합니다. */
    template <typename... Args>
    Iterator Emplace(Iterator where, Args&&... args);

    /**
     * 지정된 이터레이터가 가리키는 요소를 제거합니다.
     * @param where 제거할 요소를 가리키는 이터레이터
     * @return 제거된 요소 다음을 가리키는 이터레이터
     */
    Iterator Remove(Iterator where);

    /**
     * 특정 값과 일치하는 첫 번째 요소를 찾아 그 이터레이터를 반환합니다.
     * @param value 검색할 값
     * @return 요소를 찾으면 해당 이터레이터, 없으면 end() 이터레이터
     */
    [[nodiscard]] Iterator Find(const ValueType& value);
    [[nodiscard]] ConstIterator Find(const ValueType& value) const;

    /**
     * 특정 값과 일치하는 모든 원소를 제거합니다.
     * @return 제거된 원소의 개수
     */
    SizeType Remove(const ValueType& value);

    /**
     * 조건자를 만족하는 모든 원소를 제거합니다.
     * @return 제거된 원소의 개수
     */
    template <typename Predicate>
        requires std::predicate<Predicate, const T&>
    SizeType RemoveIf(Predicate&& pred);

public:
    // --- 이터레이터 ---
    [[nodiscard]] Iterator begin() noexcept;
    [[nodiscard]] Iterator end() noexcept;
    [[nodiscard]] ConstIterator begin() const noexcept;
    [[nodiscard]] ConstIterator end() const noexcept;

private:
    void Init();

    template <typename... Args>
    Node* CreateNode(Args&&... args);

    void DestroyNode(Node* node);

private:
    Node* head = nullptr;
    SizeType size = 0;
    [[no_unique_address]] NodeAllocatorType node_allocator;
};

template <typename T>
struct LinkedListNode
{
    LinkedListNode* prev = nullptr;
    LinkedListNode* next = nullptr;
    T value;

    template <typename... Args>
    LinkedListNode(Args&&... args)
        : value(std::forward<Args>(args)...)
    {
    }
};
}

#include "SimpleEngine/Core/Container/LinkedList.inl"
