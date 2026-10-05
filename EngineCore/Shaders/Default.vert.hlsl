#pragma se_shader vertex VSMain

#include "Bindings.hlsli"
#include "Default.hlsli"

// per-pass (뷰포트/카메라 공유, 프레임당 1회 변경)
cbuffer PassUBO : SE_VS_UNIFORM(SE_SLOT_PASS)
{
    float4x4 vp; // View-Projection 행렬 (월드 좌표 -> 카메라 화면 좌표로 변환)
}

// per-object (드로우콜마다 변경)
cbuffer ObjectUBO : SE_VS_UNIFORM(SE_VS_SLOT_OBJECT)
{
    float4x4 model; // Model 행렬 (물체의 로컬 좌표 -> 월드 공간 좌표로 변환)
    uint entity_id; // 엔진 내부의 엔티티 ID (마우스 피킹 등의 처리를 위해 전달)
}

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    // 1. Position Transform (Local -> World -> Clip Space)
    float4 world_pos4 = mul(model, float4(input.position, 1.0f));
    output.position   = mul(vp, world_pos4);
    output.world_pos  = world_pos4.xyz;

    // 2. Vector Transform
    // 비균등 스케일(Non-uniform scale) 객체의 경우 법선이 왜곡될 수 있으므로,
    // 추후 필요하다면 C++에서 transpose(inverse(Model))을 계산해 별도로 넘겨받아야 함.

    // 일단 지금은 균등 스케일(Uniform scale)이라 가정하고 Model의 3x3 회전/스케일만 추출해 사용
    float3x3 normal_matrix = (float3x3)model;

    output.world_normal  = normalize(mul(normal_matrix, input.normal));
    output.world_tangent = float4(normalize(mul(normal_matrix, input.tangent.xyz)), input.tangent.w);
    output.local_normal  = input.normal;

    // 3. Texture UV & Data
    output.tex_coord = input.tex_coord;
    output.entity_id = entity_id;

    return output;
}
