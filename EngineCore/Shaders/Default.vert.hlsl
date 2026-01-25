// VS에서 Uniform Buffer의 space 설정
// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
cbuffer UBO : register(b0, space1)
{
    float4x4 MVP;
}

struct VertexInput
{
    // C++: location 0 (Vector3f position)
    float3 position : POSITION;

    // C++: location 1 (Vector3f normal)
    float3 normal : NORMAL;

    // C++: location 2 (Vector2f tex_coord)
    float2 tex_coord : TEXCOORD0;

    // C++: location 3 (Vector4f tangent)
    float4 tangent : TANGENT;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;

    // Local -> Clip Space
    output.position = mul(MVP, float4(input.position, 1.0f));

    // 일단 임시로 Normal값을 Color로 사용
    output.color = float4(input.normal * 0.5f + 0.5f, 1.0f);

    return output;
}
