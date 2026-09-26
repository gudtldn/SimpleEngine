#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Serialization/PackedArchive.h"
#include "SimpleEngine/Core/Serialization/SerializeContext.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"

using namespace se;

// SerializeContext가 서비스를 타입으로 찾는지, archive가 호출자가 넣은 context를 트레이트에 넘기는지 검증
namespace se_serialize_context_test
{
/** context에 넣는 서비스 */
struct AddedService
{
};

/** context에 넣지 않는 서비스 */
struct MissingService
{
};
} // namespace se_serialize_context_test


TEST(SerializeContextTest, FindReturnsAddedServiceByType)
{
    using namespace se_serialize_context_test;

    SerializeContext context;
    EXPECT_EQ(context.Find<AddedService>(), nullptr);

    // 서비스를 소유하지 않고 넣은 객체를 그대로 가리킴
    AddedService service;
    context.Add(service);
    EXPECT_EQ(context.Find<AddedService>(), &service);
    EXPECT_EQ(context.Find<MissingService>(), nullptr);
}

TEST(SerializeContextTest, ArchiveCarriesContext)
{
    SerializeContext context;

    Array<u8> buffer;
    PackedWriter packed_writer(buffer);
    PackedReader packed_reader(buffer);
    toml::table table;
    TomlWriter toml_writer(table);
    TomlReader toml_reader(table);

    // 넣지 않으면 nullptr이고, 넣으면 그 context
    const auto expect_carries_context = [&context](Archive& archive)
    {
        EXPECT_EQ(archive.GetContext(), nullptr);
        archive.SetContext(&context);
        EXPECT_EQ(archive.GetContext(), &context);
    };
    expect_carries_context(packed_writer);
    expect_carries_context(packed_reader);
    expect_carries_context(toml_writer);
    expect_carries_context(toml_reader);
}
