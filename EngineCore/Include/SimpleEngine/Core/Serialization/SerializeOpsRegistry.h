#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Reflection/TypeId.h"
#include "SimpleEngine/Core/Serialization/SerializeTraits.h"
#include "SimpleEngine/Utility/Common.h"


namespace se
{
/**
 * 타입 하나에 대한 직렬화 연산
 */
struct SerializeOps
{
    void (*write)(ArchiveWriter& writer, const void* value) = nullptr;
    void (*read)(ArchiveReader& reader, void* value) = nullptr;

    /** SerializeTraits의 FORMAT_VERSION */
    u32 format_version = 0;
};

namespace detail
{
/** SerializeTraits<T>를 타입 소거된 SerializeOps로 만듭니다. */
template <typename T>
    requires HasSerializeTraits<T>
SerializeOps MakeSerializeOps()
{
    return {
        .write = [](ArchiveWriter& writer, const void* value) static
        {
            SerializeTraits<T>::Write(writer, *static_cast<const T*>(value));
        },
        .read = [](ArchiveReader& reader, void* value) static
        {
            SerializeTraits<T>::Read(reader, *static_cast<T*>(value));
        },
        .format_version = SerializeTraits<T>::FORMAT_VERSION,
    };
}
} // namespace detail

/**
 * TypeId로 SerializeOps를 찾는 전역 레지스트리
 * @note 단일 스레드 사용을 전제로 설계되었습니다.
 */
class SE_CORE_API SerializeOpsRegistry
{
    SerializeOpsRegistry() = default;

public:
    /** 전역 싱글톤 인스턴스를 가져옵니다. */
    [[nodiscard]] static SerializeOpsRegistry& Get();

    /** 주어진 타입의 SerializeOps를 등록합니다. 같은 TypeId가 이미 있으면 SE_ASSERT_RELEASE로 멈춥니다(덮어쓰지 않음). */
    void Install(TypeId id, const SerializeOps& ops);

    /** TypeId로 SerializeOps를 찾습니다. (등록되지 않았다면 NullOpt) */
    [[nodiscard]] Optional<const SerializeOps&> Find(TypeId id) const;

private:
    HashMap<TypeId, SerializeOps> ops_map;
};
} // namespace se

/**
 * type의 SerializeTraits를 SerializeOpsRegistry에 등록합니다.
 * @warning .cpp 파일에서만 사용하세요
 */
#define SE_REGISTER_SERIALIZE_TRAITS(type) \
    static_assert(::se::HasSerializeTraits<type>, \
        "SE_REGISTER_SERIALIZE_TRAITS(" #type "): SerializeTraits<" #type "> must provide " \
        "FORMAT_VERSION, Write(ArchiveWriter&, const T&) and Read(ArchiveReader&, T&)."); \
    namespace { [[maybe_unused]] const bool SE_CONCAT_NAME(_se_serialize_kick_, __LINE__) = \
        (::se::SerializeOpsRegistry::Get().Install(::se::TypeId::Of<type>(), ::se::detail::MakeSerializeOps<type>()), true); }
