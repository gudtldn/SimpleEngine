// VS에서 Uniform Buffer의 space 설정
// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks

// per-pass (뷰포트/카메라 공유)
cbuffer PassUBO : register(b0, space1)
{
    float4x4 VP;
}

// per-object (드로우콜마다 교체)
cbuffer ObjectUBO : register(b1, space1)
{
    float4x4 Model;
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

    // Local -> World -> Clip Space
    float4 world_pos = mul(Model, float4(input.position, 1.0f));
    output.position = mul(VP, world_pos);

    // 일단 임시로 Normal값을 Color로 사용
    output.color = float4(input.normal * 0.5f + 0.5f, 1.0f);

    return output;
}
