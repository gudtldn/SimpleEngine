// 문법 오류로 컴파일에 실패합니다.

[shader("vertex")]
float4 VSMain(uint vertex_id : SV_VertexID) : SV_Position
{
    return float4(vertex_id 0.0f, 0.0f, 1.0f);
}
