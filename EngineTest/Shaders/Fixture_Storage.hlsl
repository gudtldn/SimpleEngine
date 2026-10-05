// 같은 set에서 텍스처 다음 번호에 읽기 전용 스토리지 버퍼를 선언합니다.

StructuredBuffer<float4> vertex_offsets : register(t0, space0);

Texture2D albedo_texture : register(t0, space2);
Texture2D normal_texture : register(t1, space2);
SamplerState albedo_sampler : register(s0, space2);
StructuredBuffer<float4> lights : register(t2, space2);
ByteAddressBuffer light_indices : register(t3, space2);

[shader("vertex")]
float4 VSMain(uint vertex_id : SV_VertexID) : SV_Position
{
    return vertex_offsets[vertex_id];
}

[shader("pixel")]
float4 PSMain(float4 position : SV_Position) : SV_Target0
{
    const float2 uv = position.xy;
    const uint light_index = light_indices.Load(0);
    return albedo_texture.Sample(albedo_sampler, uv) + normal_texture.Sample(albedo_sampler, uv) + lights[light_index];
}
