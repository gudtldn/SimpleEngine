export module SE.Types:Containers;

import std;

#define SE_USING_STD_PMR_CONTAINER(container_name) \
using container_name = std::pmr::container_name


// pmr container wrapper
export namespace se
{
template <typename T, size_t N>
using array = std::array<T, N>;

template <typename T>
SE_USING_STD_PMR_CONTAINER(vector)<T>;

template <typename T>
SE_USING_STD_PMR_CONTAINER(list)<T>;

template <typename T>
SE_USING_STD_PMR_CONTAINER(deque)<T>;

template <typename T>
using queue = std::queue<T, deque<T>>;

template <typename T>
using stack = std::stack<T, deque<T>>;

template <typename T, typename Pred = std::less<T>>
SE_USING_STD_PMR_CONTAINER(set)<T, Pred>;

template <typename T, typename Hasher = std::hash<T>, typename KeyEq = std::equal_to<T>>
SE_USING_STD_PMR_CONTAINER(unordered_set)<T, Hasher, KeyEq>;

template <typename Key, typename Value, typename Pred = std::less<Key>>
SE_USING_STD_PMR_CONTAINER(map)<Key, Value, Pred>;

template <typename Key, typename Value, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
SE_USING_STD_PMR_CONTAINER(unordered_map)<Key, Value>;

// 문자열
SE_USING_STD_PMR_CONTAINER(string);
SE_USING_STD_PMR_CONTAINER(wstring);
SE_USING_STD_PMR_CONTAINER(u8string);
SE_USING_STD_PMR_CONTAINER(u16string);
SE_USING_STD_PMR_CONTAINER(u32string);
}
