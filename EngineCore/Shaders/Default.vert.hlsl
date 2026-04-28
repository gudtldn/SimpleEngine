// VS에서 Uniform Buffer의 space 설정
// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks

#pragma se_shader vertex VSMain

#include "Default.hlsli"

// per-pass (뷰포트/카메라 공유, 프레임당 1회 변경)
cbuffer PassUBO : register(b0, space1)
{
    float4x4 VP; // View-Projection 행렬 (월드 좌표 -> 카메라 화면 좌표로 변환)
}

// per-object (드로우콜마다 변경)
cbuffer ObjectUBO : register(b1, space1)
{
    float4x4 Model; // Model 행렬 (물체의 로컬 좌표 -> 월드 공간 좌표로 변환)
    uint EntityId;  // 엔진 내부의 엔티티 ID (마우스 피킹 등의 처리를 위해 전달)
}

VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    // Local -> World -> Clip Space
    float4 world_pos = mul(Model, float4(input.position, 1.0f));
    output.position = mul(VP, world_pos);

    // VS -> PS 패스스루
    output.tex_coord = input.tex_coord;
    output.entity_id = EntityId;

    return output;
}
