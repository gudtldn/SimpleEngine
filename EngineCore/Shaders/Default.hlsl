#include "Bindings.hlsli"
#include "Default.hlsli"

// ==================== Vertex ====================

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

[shader("vertex")]
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

// ==================== Fragment ====================

// Per-Pass: 프레임당 1회 업데이트 (카메라, 글로벌 조명 등)
// std140 레이아웃 (총 48 Bytes)
cbuffer SceneDataUBO : SE_PS_UNIFORM(SE_SLOT_PASS)
{
    float3 camera_pos;     // [0:11]  카메라 월드 위치 (Specular 계산용)
    float  _pad0;          // [12:15] 패딩
    float3 light_dir_ws;   // [16:27] 태양광 방향 벡터 (빛을 향하는 방향, 정규화됨)
    float  _pad1;          // [28:31] 패딩
    float3 light_color;    // [32:43] 주광원 색상 * 강도(Intensity)
    uint   rendering_mode; // [44:47] 렌더링 모드 (0=Lit, 1=Unlit, 2=Wireframe, 3=Normal, 4=WorldNormal)
}

// Per-Material: 머티리얼 인스턴스 교체 시 업데이트
// std140 레이아웃 (총 48 Bytes)
cbuffer MaterialUBO : SE_PS_UNIFORM(SE_PS_SLOT_MATERIAL)
{
    float4 base_color_factor; // [0:15]  베이스 컬러 팩터 (RGBA)
    float  metallic_factor;   // [16:19] 메탈릭 팩터 (0.0=비금속, 1.0=금속)
    float  roughness_factor;  // [20:23] 러프니스 팩터 (0.0=매끄러움, 1.0=거침)
    float  alpha_cutoff;      // [24:27] 알파 마스크 임계값
    uint   flags;             // [28:31] 비트 플래그 (bit0 = alpha test 활성화)
    float4 emissive_factor;   // [32:47] 이미시브 팩터 (xyz 사용, w=0 예약)
}

// glTF PBR Metallic-Roughness 슬롯
// t/s 인덱스는 Material::texture_slots[].fragment_slot과 일치해야 함
Texture2D<float4> BaseColorTexture         : SE_PS_TEXTURE(0);
SamplerState      BaseColorSampler         : SE_PS_SAMPLER(0);

Texture2D<float4> MetallicRoughnessTexture : SE_PS_TEXTURE(1);
SamplerState      MetallicRoughnessSampler : SE_PS_SAMPLER(1);

Texture2D<float4> NormalTexture            : SE_PS_TEXTURE(2);
SamplerState      NormalSampler            : SE_PS_SAMPLER(2);

Texture2D<float4> OcclusionTexture         : SE_PS_TEXTURE(3);
SamplerState      OcclusionSampler         : SE_PS_SAMPLER(3);

Texture2D<float4> EmissiveTexture          : SE_PS_TEXTURE(4);
SamplerState      EmissiveSampler          : SE_PS_SAMPLER(4);

// MRT (Multiple Render Targets) 레이아웃
struct PSOutput
{
    float4 color     : SV_Target0; // RT0: 씬 컬러 버퍼
    uint   entity_id : SV_Target1; // RT1: 엔티티 ID 버퍼
};

[shader("pixel")]
PSOutput PSMain(VertexOutput input)
{
    PSOutput output;

    // Picking용 EntityID 설정
    output.entity_id = input.entity_id;

    // 1. Texture Sampling
    float4 base_color = BaseColorTexture.Sample(BaseColorSampler, input.tex_coord) * base_color_factor;
    float4 emissive   = EmissiveTexture.Sample(EmissiveSampler, input.tex_coord)   * emissive_factor;

    // 2. Alpha Testing (Masked Blend Mode)
    // flags의 0번째 비트가 1이고, 알파 값이 임계값보다 낮으면 픽셀 폐기
    if ((flags & 0x1u) != 0u && base_color.a < alpha_cutoff)
    {
        discard;
    }

    // ----- rendering_mode: 0=Lit, 1=Unlit, 2=Wireframe, 3=Normal, 4=WorldNormal -----

    // Lit / Wireframe
    if (rendering_mode == 0u || rendering_mode == 2u)
    {
        output.color = float4(base_color.rgb + emissive.rgb, base_color.a);
    }

    // Unlit
    else if (rendering_mode == 1u)
    {
        output.color = float4(base_color.rgb + emissive.rgb, base_color.a);
    }

    // Normal (오브젝트 공간 노멀 시각화)
    else if (rendering_mode == 3u)
    {
        output.color = float4(input.local_normal * 0.5f + 0.5f, 1.0f);
    }

    // WorldNormal (정규화된 월드 노멀 시각화)
    else if (rendering_mode == 4u)
    {
        output.color = float4(normalize(input.world_normal) * 0.5f + 0.5f, 1.0f);
    }

    // -------------------- 밉맵 디버그 시각화 --------------------
//     // 1. GPU가 이 픽셀에서 사용하기로 결정한 LOD(층수)를 역으로 알아냅니다.
//     float calculated_lod = BaseColorTexture.CalculateLevelOfDetail(BaseColorSampler, input.tex_coord);
//
//     // 2. 소수점을 버리고 정수 층수만 추출합니다. (0, 1, 2, 3...)
//     uint mip_level = (uint)floor(calculated_lod);
//
//     // 3. 층수에 따라 원색으로 덮어버립니다. (switch-case 또는 if-else)
//     float3 debug_color = float3(1.0f, 1.0f, 1.0f); // 기본 흰색
//
//     if (mip_level == 0)      debug_color = float3(1.0f, 0.0f, 0.0f); // Mip 0: 빨강 (가장 가까움)
//     else if (mip_level == 1) debug_color = float3(0.0f, 1.0f, 0.0f); // Mip 1: 초록
//     else if (mip_level == 2) debug_color = float3(0.0f, 0.0f, 1.0f); // Mip 2: 파랑
//     else if (mip_level == 3) debug_color = float3(1.0f, 1.0f, 0.0f); // Mip 3: 노랑
//     else if (mip_level == 4) debug_color = float3(1.0f, 0.0f, 1.0f); // Mip 4: 자주
//     else                     debug_color = float3(0.0f, 1.0f, 1.0f); // Mip 5 이상: 청록
//
//     // 4. 원래 텍스처 색상 대신 디버그 색상 출력
//     output.color = float4(debug_color, 1.0f);
    // -------------------- 밉맵 디버그 시각화 --------------------

    return output;
}
