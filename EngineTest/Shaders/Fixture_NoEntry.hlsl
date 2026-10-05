// [shader] 속성이 없어 진입점이 없습니다.

float4 VSMain(uint vertex_id : SV_VertexID) : SV_Position
{
    return float4(float(vertex_id), 0.0f, 0.0f, 1.0f);
}
