// [shader] 속성이 붙은 함수가 없어 진입점을 찾지 못합니다.

float4 VSMain(uint vertex_id : SV_VertexID) : SV_Position
{
    return float4(float(vertex_id), 0.0f, 0.0f, 1.0f);
}
