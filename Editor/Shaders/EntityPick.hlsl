// Entity Pick Shader

#pragma se_shader vertex VSMain
#pragma se_shader fragment PSMain

// per-pass (뷰포트/카메라 공유)
cbuffer PassUBO : register(b0, space1)
{
    float4x4 VP;
}

// per-object (드로우콜마다 교체)
cbuffer ObjectUBO : register(b1, space1)
{
    float4x4 Model;
    uint EntityId;
}

struct VertexInput
{
    // C++: location 0 (Vector3f position)
    float3 position : POSITION;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    nointerpolation uint entity_id : TEXCOORD0;
};

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    float4 world_pos = mul(Model, float4(input.position, 1.0f));
    output.position = mul(VP, world_pos);

    output.entity_id = EntityId;

    return output;
}

uint PSMain(VertexOutput input) : SV_TARGET
{
    return input.entity_id;
}
