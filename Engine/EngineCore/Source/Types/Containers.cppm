export module SE.Types:Containers;

import std;

#define SE_USE_STD_CONTAINERS(container_name) \
template <typename T> \
using container_name = std::pmr::container_name<T>;

#define SE_USE_STD_STRING(container_name) \
using container_name = std::pmr::container_name;


export namespace se
{
// 컨테이너
SE_USE_STD_CONTAINERS(vector);
SE_USE_STD_CONTAINERS(list);
SE_USE_STD_CONTAINERS(deque);

template <typename T, typename Pred = std::less<T>>
using set = std::pmr::set<T, Pred>;

template <typename T, typename Hasher = std::hash<T>, typename KeyEq = std::equal_to<T>>
using unordered_set = std::pmr::unordered_set<T, Hasher, KeyEq>;

template <typename Key, typename Value, typename Pred = std::less<Key>>
using map = std::pmr::map<Key, Value, Pred>;;

template <typename Key, typename Value, typename Hasher = std::hash<Key>, typename KeyEq = std::equal_to<Key>>
using unordered_map = std::pmr::unordered_map<Key, Value>;;

// 문자열
SE_USE_STD_STRING(string);
SE_USE_STD_STRING(wstring);
SE_USE_STD_STRING(u8string);
SE_USE_STD_STRING(u16string);
SE_USE_STD_STRING(u32string);
}
