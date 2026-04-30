#include "gtest/gtest.h"

#include "SimpleEngine/Graphics/ShaderUtils.h"

using namespace se;
using namespace se;

class FilterVertexInputStateTest : public ::testing::Test
{
protected:
    // 4-attribute vertex layout: position(0), normal(1), texcoord(2), tangent(3)
    SDL_GPUVertexBufferDescription buffer_desc = {
        .slot = 0,
        .pitch = 48,
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    };

    SDL_GPUVertexAttribute attrs[4] = {
        { .location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 0  },
        { .location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .offset = 12 },
        { .location = 2, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 24 },
        { .location = 3, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = 32 },
    };

    SDL_GPUVertexInputState original = {
        .vertex_buffer_descriptions = &buffer_desc,
        .num_vertex_buffers = 1,
        .vertex_attributes = attrs,
        .num_vertex_attributes = 4,
    };
};

// 리플렉션이 비어있으면 원본 전체를 복사
TEST_F(FilterVertexInputStateTest, EmptyReflectionReturnsOriginal)
{
    ShaderReflectionData reflection;

    auto result = FilterVertexInputState(original, reflection);

    EXPECT_EQ(result.attributes.Len(), 4u);
    EXPECT_EQ(result.attributes[0].location, 0u);
    EXPECT_EQ(result.attributes[3].location, 3u);
    EXPECT_EQ(result.vertex_buffer_descriptions, original.vertex_buffer_descriptions);
    EXPECT_EQ(result.num_vertex_buffers, 1u);
}

// 셰이더가 4개 중 3개만 사용 (tangent 제거)
TEST_F(FilterVertexInputStateTest, FiltersUnusedAttribute)
{
    ShaderReflectionData reflection;
    reflection.vertex_inputs = { { 0 }, { 1 }, { 2 } }; // tangent(3) 없음

    auto result = FilterVertexInputState(original, reflection);

    EXPECT_EQ(result.attributes.Len(), 3u);
    EXPECT_EQ(result.attributes[0].location, 0u);
    EXPECT_EQ(result.attributes[1].location, 1u);
    EXPECT_EQ(result.attributes[2].location, 2u);
    // buffer description 변경 없음
    EXPECT_EQ(result.vertex_buffer_descriptions, original.vertex_buffer_descriptions);
    EXPECT_EQ(result.num_vertex_buffers, 1u);
}

// 셰이더가 1개만 사용 (position만)
TEST_F(FilterVertexInputStateTest, FiltersSingleAttribute)
{
    ShaderReflectionData reflection;
    reflection.vertex_inputs = { { 0 } };

    auto result = FilterVertexInputState(original, reflection);

    EXPECT_EQ(result.attributes.Len(), 1u);
    EXPECT_EQ(result.attributes[0].location, 0u);
    EXPECT_EQ(result.attributes[0].offset, 0u);
}

// 셰이더가 모든 attribute 사용 -> 필터링해도 4개 유지
TEST_F(FilterVertexInputStateTest, AllAttributesUsedKeepsAll)
{
    ShaderReflectionData reflection;
    reflection.vertex_inputs = { { 0 }, { 1 }, { 2 }, { 3 } };

    auto result = FilterVertexInputState(original, reflection);

    EXPECT_EQ(result.attributes.Len(), 4u);
}

// 리플렉션에 존재하지만 원본에 없는 location은 무시됨
TEST_F(FilterVertexInputStateTest, ExtraReflectionLocationIgnored)
{
    ShaderReflectionData reflection;
    reflection.vertex_inputs = { { 0 }, { 1 }, { 7 } }; // 7은 원본에 없음

    auto result = FilterVertexInputState(original, reflection);

    EXPECT_EQ(result.attributes.Len(), 2u);
    EXPECT_EQ(result.attributes[0].location, 0u);
    EXPECT_EQ(result.attributes[1].location, 1u);
}

// 비연속 location 필터링 (0, 3만 사용)
TEST_F(FilterVertexInputStateTest, NonContiguousLocations)
{
    ShaderReflectionData reflection;
    reflection.vertex_inputs = { { 0 }, { 3 } };

    auto result = FilterVertexInputState(original, reflection);

    EXPECT_EQ(result.attributes.Len(), 2u);
    EXPECT_EQ(result.attributes[0].location, 0u);
    EXPECT_EQ(result.attributes[0].offset, 0u);
    EXPECT_EQ(result.attributes[1].location, 3u);
    EXPECT_EQ(result.attributes[1].offset, 32u);
}

// 원본 attribute가 0개인 경우
TEST_F(FilterVertexInputStateTest, ZeroOriginalAttributes)
{
    SDL_GPUVertexInputState empty_original = {
        .vertex_buffer_descriptions = &buffer_desc,
        .num_vertex_buffers = 1,
        .vertex_attributes = nullptr,
        .num_vertex_attributes = 0,
    };

    ShaderReflectionData reflection;
    reflection.vertex_inputs = { { 0 } };

    auto result = FilterVertexInputState(empty_original, reflection);

    EXPECT_EQ(result.attributes.Len(), 0u);
}

// AsState()가 이동 후에도 올바른 포인터를 반환하는지 확인
TEST_F(FilterVertexInputStateTest, AsStatePointsIntoOwnedArray)
{
    ShaderReflectionData reflection;
    reflection.vertex_inputs = { { 0 }, { 2 } };

    FilteredVertexInputState moved;
    {
        auto tmp = FilterVertexInputState(original, reflection);
        moved = std::move(tmp);
    }

    const SDL_GPUVertexInputState state = moved.AsState();
    EXPECT_EQ(state.vertex_attributes, moved.attributes.Data());
    EXPECT_EQ(state.num_vertex_attributes, 2u);
    EXPECT_EQ(state.vertex_attributes[0].location, 0u);
    EXPECT_EQ(state.vertex_attributes[1].location, 2u);
}
