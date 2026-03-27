// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks
cbuffer UBO : register(b0, space1)
{
    float4x4 VP;
}

struct VertexInput
{
    // C++: location 0 (Vector3f position, 월드 공간 좌표)
    float3 position : POSITION;

    // C++: location 1 (LinearColor color)
    float4 color : COLOR;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.position = mul(VP, float4(input.position, 1.0f));
    output.color = input.color;
    return output;
}
