// 컴퓨트 스테이지의 세 set(읽기 전용, 읽기/쓰기, 상수 버퍼)을 모두 사용합니다.

Texture2D<float4> source_texture : register(t0, space0);
SamplerState source_sampler : register(s0, space0);
StructuredBuffer<float4> input_particles : register(t1, space0);

RWTexture2D<float4> output_texture : register(u0, space1);
RWStructuredBuffer<float4> output_particles : register(u1, space1);

cbuffer DispatchUBO : register(b0, space2)
{
    float delta_time;
    uint particle_count;
}

[shader("compute")]
[numthreads(8, 8, 1)]
void CSMain(uint3 thread_id : SV_DispatchThreadID)
{
    if (thread_id.x >= particle_count)
    {
        return;
    }

    const float4 color = source_texture.SampleLevel(source_sampler, float2(thread_id.xy) / 8.0f, 0.0f);
    output_texture[thread_id.xy] = color;
    output_particles[thread_id.x] = input_particles[thread_id.x] + color * delta_time;
}
