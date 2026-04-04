// FS에서 Uniform Buffer의 space 설정
// https://wiki.libsdl.org/SDL3/SDL_CreateGPUShader#remarks

struct PixelInput
{
    float4 position : SV_POSITION;
    float2 tex_coord : TEXCOORD0;
    float4 color : COLOR;
};

float4 main(PixelInput input) : SV_Target0
{
    // 보간된 색상을 그대로 반환
    return input.color;
}
