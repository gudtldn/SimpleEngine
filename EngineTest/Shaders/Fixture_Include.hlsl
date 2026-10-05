// 다른 파일을 포함해 의존 목록에 두 파일이 들어갑니다.
#include "Fixture_Common.hlsli"

[shader("vertex")]
float4 VSMain(uint vertex_id : SV_VertexID) : SV_Position
{
    return FullscreenPosition(vertex_id);
}

[shader("pixel")]
float4 PSMain() : SV_Target0
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
