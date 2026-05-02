// Default 셰이더 공유 타입 정의 (VS/FS 공통)

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
    float2 tex_coord : TEXCOORD0;
    nointerpolation uint entity_id : TEXCOORD1;
};