// Default 셰이더 공유 타입 정의 (VS/FS 공통)

struct VertexInput
{
    float3 position : POSITION;   // C++: location 0 (Vector3f position)
    float3 normal : NORMAL;       // C++: location 1 (Vector3f normal)
    float2 tex_coord : TEXCOORD0; // C++: location 2 (Vector2f tex_coord)
    float4 tangent : TANGENT;     // C++: location 3 (Vector4f tangent)
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
