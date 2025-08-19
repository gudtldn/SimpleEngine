struct PixelInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

float4 main(PixelInput input) : SV_Target0
{
    int px = int(input.position.x);
    int py = int(input.position.y);

    if (px % 2 == 0 || py % 2 == 0)
    {
        discard;
    }

    // 보간된 색상을 그대로 반환
    return input.color;
}
