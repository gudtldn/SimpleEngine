// 샘플러가 s0~s2를 차지하는데, 스토리지 버퍼를 그 범위 안쪽인 t2에 선언합니다.

Texture2D albedo_texture : register(t0, space2);
SamplerState point_sampler : register(s0, space2);
SamplerState linear_sampler : register(s1, space2);
SamplerState anisotropic_sampler : register(s2, space2);
StructuredBuffer<float4> lights : register(t2, space2);

[shader("pixel")]
float4 PSMain(float4 position : SV_Position) : SV_Target0
{
    return lights[0];
}
