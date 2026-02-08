struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PixelInput input) : SV_Target0
{
    // 보간된 색상을 그대로 반환
    return input.color;
}
