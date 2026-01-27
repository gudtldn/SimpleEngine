#pragma once
#include <cassert>


namespace se
{
template <typename T>
LinkedListIterator<T>::reference LinkedListIterator<T>::operator*() const
{
    assert(node_ptr != nullptr && "Dereferencing a null iterator.");
    return node_ptr->value;
}

template <typename T>
LinkedListIterator<T>::pointer LinkedListIterator<T>::operator->() const
{
    assert(node_ptr != nullptr && "Dereferencing a null iterator.");
    return std::addressof(node_ptr->value);
}

template <typename T>
LinkedListIterator<T>& LinkedListIterator<T>::operator++()
{
    assert(node_ptr != nullptr && "Incrementing a null iterator.");
    node_ptr = node_ptr->next;
    return *this;
}

template <typename T>
LinkedListIterator<T> LinkedListIterator<T>::operator++(int)
{
    LinkedListIterator temp = *this;
    ++(*this);
    return temp;
}

template <typename T>
LinkedListIterator<T>& LinkedListIterator<T>::operator--()
{
    assert(node_ptr != nullptr && "Decrementing a null iterator.");
    node_ptr = node_ptr->prev;
    return *this;
}

template <typename T>
LinkedListIterator<T> LinkedListIterator<T>::operator--(int)
{
    LinkedListIterator temp = *this;
    --(*this);
    return temp;
}

template <typename T>
ConstLinkedListIterator<T>::ConstLinkedListIterator(const LinkedListIterator<T>& other)
    : node_ptr(other.node_ptr)
{
}

template <typename T>
ConstLinkedListIterator<T>::reference ConstLinkedListIterator<T>::operator*() const
{
    assert(node_ptr != nullptr && "Dereferencing a null iterator.");
    return node_ptr->value;
}

template <typename T>
ConstLinkedListIterator<T>::pointer ConstLinkedListIterator<T>::operator->() const
{
    assert(node_ptr != nullptr && "Dereferencing a null iterator.");
    return std::addressof(node_ptr->value);
}

template <typename T>
ConstLinkedListIterator<T>& ConstLinkedListIterator<T>::operator++()
{
    assert(node_ptr != nullptr && "Incrementing a null iterator.");
    node_ptr = node_ptr->next;
    return *this;
}

template <typename T>
ConstLinkedListIterator<T> ConstLinkedListIterator<T>::operator++(int)
{
    ConstLinkedListIterator temp = *this;
    ++(*this);
    return temp;
}

template <typename T>
ConstLinkedListIterator<T>& ConstLinkedListIterator<T>::operator--()
{
    assert(node_ptr != nullptr && "Decrementing a null iterator.");
    node_ptr = node_ptr->prev;
    return *this;
}

template <typename T>
ConstLinkedListIterator<T> ConstLinkedListIterator<T>::operator--(int)
{
    ConstLinkedListIterator temp = *this;
    --(*this);
    return temp;
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::LinkedList_DEPRECATED() noexcept
{
    Init();
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::LinkedList_DEPRECATED(SizeType count)
{
    Init();
    for (SizeType i = 0; i < count; ++i)
    {
        EmplaceBack();
    }
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::LinkedList_DEPRECATED(SizeType count, const ValueType& value)
{
    Init();
    for (SizeType i = 0; i < count; ++i)
    {
        PushBack(value);
    }
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::LinkedList_DEPRECATED(std::initializer_list<ValueType> init_list)
{
    Init();
    for (const auto& value : init_list)
    {
        PushBack(value);
    }
}

template <typename T, typename Allocator>
template <std::input_iterator It, std::sentinel_for<It> Sent>
    requires std::same_as<std::iter_value_t<It>, T>
LinkedList_DEPRECATED<T, Allocator>::LinkedList_DEPRECATED(It first, Sent last)
{
    Init();
    for (auto it = first; it != last; ++it)
    {
        PushBack(*it);
    }
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::~LinkedList_DEPRECATED()
{
    Clear();
    NodeAllocTraits::deallocate(node_allocator, head, 1);
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::LinkedList_DEPRECATED(const LinkedList_DEPRECATED& other)
{
    Init();
    for (const auto& value : other)
    {
        PushBack(value);
    }
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>& LinkedList_DEPRECATED<T, Allocator>::operator=(const LinkedList_DEPRECATED& other)
{
    if (this != &other)
    {
        Clear();
        for (const auto& value : other)
        {
            PushBack(value);
        }
    }
    return *this;
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::LinkedList_DEPRECATED(LinkedList_DEPRECATED&& other) noexcept
    : head(other.head)
    , size(other.size)
    , node_allocator(std::move(other.node_allocator))
{
    other.head = nullptr;
    other.size = 0;
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>& LinkedList_DEPRECATED<T, Allocator>::operator=(LinkedList_DEPRECATED&& other) noexcept
{
    if (this != &other)
    {
        Clear();
        NodeAllocTraits::deallocate(node_allocator, head, 1);

        head = other.head;
        size = other.size;
        node_allocator = std::move(other.node_allocator);

        other.head = nullptr;
        other.size = 0;
    }
    return *this;
}

template <typename T, typename Allocator>
template <std::ranges::input_range Rng>
    requires std::same_as<std::ranges::range_value_t<Rng>, T>
LinkedList_DEPRECATED<T, Allocator> LinkedList_DEPRECATED<T, Allocator>::FromRange(Rng&& range)
{
    return LinkedList_DEPRECATED{ std::ranges::begin(range), std::ranges::end(range) };
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::SizeType LinkedList_DEPRECATED<T, Allocator>::Len() const noexcept
{
    return size;
}

template <typename T, typename Allocator>
bool LinkedList_DEPRECATED<T, Allocator>::IsEmpty() const noexcept
{
    return size == 0;
}

template <typename T, typename Allocator>
void LinkedList_DEPRECATED<T, Allocator>::Clear()
{
    if (IsEmpty())
    {
        return;
    }

    Node* current = head->next;
    while (current != head)
    {
        Node* next = current->next;
        DestroyNode(current);
        current = next;
    }
    head->next = head;
    head->prev = head;
    size = 0;
}

template <typename T, typename Allocator>
Optional<T&> LinkedList_DEPRECATED<T, Allocator>::Front()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    return head->next->value;
}

template <typename T, typename Allocator>
Optional<const T&> LinkedList_DEPRECATED<T, Allocator>::Front() const
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    return head->next->value;
}

template <typename T, typename Allocator>
Optional<T&> LinkedList_DEPRECATED<T, Allocator>::Back()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    return head->prev->value;
}

template <typename T, typename Allocator>
Optional<const T&> LinkedList_DEPRECATED<T, Allocator>::Back() const
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    return head->prev->value;
}

template <typename T, typename Allocator>
void LinkedList_DEPRECATED<T, Allocator>::PushFront(const ValueType& value)
{
    Emplace(begin(), value);
}

template <typename T, typename Allocator>
void LinkedList_DEPRECATED<T, Allocator>::PushFront(ValueType&& value)
{
    Emplace(begin(), std::move(value));
}

template <typename T, typename Allocator>
void LinkedList_DEPRECATED<T, Allocator>::PushBack(const ValueType& value)
{
    Emplace(end(), value);
}

template <typename T, typename Allocator>
void LinkedList_DEPRECATED<T, Allocator>::PushBack(ValueType&& value)
{
    Emplace(end(), std::move(value));
}

template <typename T, typename Allocator>
Optional<T> LinkedList_DEPRECATED<T, Allocator>::PopFront()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    T value = std::move(head->next->value);
    Remove(begin());
    return value;
}

template <typename T, typename Allocator>
Optional<T> LinkedList_DEPRECATED<T, Allocator>::PopBack()
{
    if (IsEmpty())
    {
        return std::nullopt;
    }

    T value = std::move(head->prev->value);
    Remove(--end());
    return value;
}

template <typename T, typename Allocator>
template <typename... Args>
T& LinkedList_DEPRECATED<T, Allocator>::EmplaceFront(Args&&... args)
{
    return *Emplace(begin(), std::forward<Args>(args)...);
}

template <typename T, typename Allocator>
template <typename... Args>
T& LinkedList_DEPRECATED<T, Allocator>::EmplaceBack(Args&&... args)
{
    return *Emplace(end(), std::forward<Args>(args)...);
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::Iterator LinkedList_DEPRECATED<T, Allocator>::Insert(Iterator where, const ValueType& value)
{
    return Emplace(where, value);
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::Iterator LinkedList_DEPRECATED<T, Allocator>::Insert(Iterator where, ValueType&& value)
{
    return Emplace(where, std::move(value));
}

template <typename T, typename Allocator>
template <typename... Args>
LinkedList_DEPRECATED<T, Allocator>::Iterator LinkedList_DEPRECATED<T, Allocator>::Emplace(Iterator where, Args&&... args)
{
    Node* current_node = where.node_ptr;
    Node* prev_node = current_node->prev;
    Node* new_node = CreateNode(std::forward<Args>(args)...);

    prev_node->next = new_node;
    new_node->prev = prev_node;
    new_node->next = current_node;
    current_node->prev = new_node;

    size++;
    return Iterator{ new_node };
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::Iterator LinkedList_DEPRECATED<T, Allocator>::Remove(Iterator where)
{
    assert(where.node_ptr != head && "Cannot remove the end() iterator.");
    assert(!IsEmpty() && "Cannot remove from an empty list.");

    Node* node_to_remove = where.node_ptr;
    Node* prev_node = node_to_remove->prev;
    Node* next_node = node_to_remove->next;

    prev_node->next = next_node;
    next_node->prev = prev_node;

    DestroyNode(node_to_remove);
    size--;

    return Iterator{ next_node };
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::Iterator LinkedList_DEPRECATED<T, Allocator>::Find(const ValueType& value)
{
    for (auto it = begin(); it != end(); ++it)
    {
        if (*it == value)
        {
            return it;
        }
    }
    return end();
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::ConstIterator LinkedList_DEPRECATED<T, Allocator>::Find(const ValueType& value) const
{
    for (auto it = begin(); it != end(); ++it)
    {
        if (*it == value)
        {
            return it;
        }
    }
    return end();
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::SizeType LinkedList_DEPRECATED<T, Allocator>::Remove(const ValueType& value)
{
    return RemoveIf([&value](const T& elem)
    {
        return elem == value;
    });
}

template <typename T, typename Allocator>
template <typename Predicate>
    requires std::predicate<Predicate, const T&>
LinkedList_DEPRECATED<T, Allocator>::SizeType LinkedList_DEPRECATED<T, Allocator>::RemoveIf(Predicate&& pred)
{
    SizeType count = 0;
    for (auto it = begin(); it != end();)
    {
        if (pred(*it))
        {
            it = Remove(it);
            count++;
        }
        else
        {
            ++it;
        }
    }
    return count;
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::Iterator LinkedList_DEPRECATED<T, Allocator>::begin() noexcept
{
    return Iterator{ head->next };
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::Iterator LinkedList_DEPRECATED<T, Allocator>::end() noexcept
{
    return Iterator{ head };
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::ConstIterator LinkedList_DEPRECATED<T, Allocator>::begin() const noexcept
{
    return ConstIterator{ head->next };
}

template <typename T, typename Allocator>
LinkedList_DEPRECATED<T, Allocator>::ConstIterator LinkedList_DEPRECATED<T, Allocator>::end() const noexcept
{
    return ConstIterator{ head };
}

template <typename T, typename Allocator>
void LinkedList_DEPRECATED<T, Allocator>::Init()
{
    head = NodeAllocTraits::allocate(node_allocator, 1);
    head->prev = head;
    head->next = head;
    size = 0;
}

template <typename T, typename Allocator>
template <typename... Args>
LinkedList_DEPRECATED<T, Allocator>::Node* LinkedList_DEPRECATED<T, Allocator>::CreateNode(Args&&... args)
{
    Node* new_node = NodeAllocTraits::allocate(node_allocator, 1);
    NodeAllocTraits::construct(node_allocator, new_node, std::forward<Args>(args)...);
    return new_node;
}

template <typename T, typename Allocator>
void LinkedList_DEPRECATED<T, Allocator>::DestroyNode(Node* node)
{
    NodeAllocTraits::destroy(node_allocator, node);
    NodeAllocTraits::deallocate(node_allocator, node, 1);
}
}  // namespace se
