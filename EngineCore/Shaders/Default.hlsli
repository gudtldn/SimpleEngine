// Default 셰이더 공유 타입 정의 (VS/FS 공통)

struct VertexInput
{
    [[vk::location(0)]] float3 position  : TEXCOORD0; // C++: Vector3f position
    [[vk::location(1)]] float3 normal    : TEXCOORD1; // C++: Vector3f normal
    [[vk::location(2)]] float2 tex_coord : TEXCOORD2; // C++: Vector2f tex_coord
    [[vk::location(3)]] float4 tangent   : TEXCOORD3; // C++: Vector4f tangent
};

struct VertexOutput
{
    // Clip Space
    float4 position      : SV_POSITION; // GPU 래스터라이저가 사용하는 화면 투영 좌표

    // World Space
    float3 world_pos     : TEXCOORD0; // 조명 계산용 실제 3D 위치
    float3 world_normal  : TEXCOORD1; // 조명 계산용 법선 벡터
    float4 world_tangent : TEXCOORD2; // 노멀맵용 접선 벡터 (w = bitangent 부호)

    // Local Space
    float3 local_normal  : TEXCOORD3; // 오브젝트 공간 법선 (Normal 디버그 모드용)

    // UV / Data
    float2 tex_coord     : TEXCOORD4; // 텍스처 좌표
    nointerpolation uint entity_id : TEXCOORD5; // 피킹용 엔티티 ID (픽셀 간 보간 방지)
};
