// 헤더를 include하므로 의존 파일이 두 개입니다.
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
