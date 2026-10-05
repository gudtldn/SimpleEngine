// 스테이지 set 규약을 어기는 선언: UBO set의 텍스처, 그래픽스 스테이지의 RW 텍스처, 어느 스테이지도 쓰지 않는 space

cbuffer PassUBO : register(b0, space1)
{
    float4x4 view_projection;
}

Texture2D albedo_texture : register(t1, space1);
SamplerState albedo_sampler : register(s0, space2);
RWTexture2D<float4> output_texture : register(u1, space2);
StructuredBuffer<float4> stray_buffer : register(t0, space5);

[shader("vertex")]
float4 VSMain(uint vertex_id : SV_VertexID) : SV_Position
{
    return mul(view_projection, float4(float(vertex_id), 0.0f, 0.0f, 1.0f));
}

[shader("pixel")]
float4 PSMain(float4 position : SV_Position) : SV_Target0
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
