// ==============================================================================
// 무한 그리드 셰이더 (Infinite Grid Shader)
// - 깊이 테스트(Depth Test) 비활성화: 항상 다른 씬 오브젝트들 위에 또는 배경으로 그려짐
// - 블렌딩(Alpha Blending) 활성화: 투명도를 이용해 그리드 선을 표현함
// ==============================================================================

#pragma se_shader vertex VSMain
#pragma se_shader fragment PSMain

// ----------------------------------------------------------------
// [정점 셰이더 상수 버퍼]
// ----------------------------------------------------------------
cbuffer VS_UBO : register(b0, space1)
{
    float4x4 VP;      // 뷰-투영 행렬 (Row-major -> Column-major 자동 전치)
    float3 CameraPos; // 현재 카메라의 월드 좌표
    float GridSize;   // 무한 그리드를 그릴 기본 바탕 평면(Quad)의 크기
}

// ----------------------------------------------------------------
// [픽셀 셰이더 상수 버퍼]
// ----------------------------------------------------------------
cbuffer PS_UBO : register(b0, space3)
{
    // 선이 너무 촘촘해져서 다음 단계의 굵은 선(LOD)으로 넘어가기 전,
    // 그리드 한 칸이 화면에서 유지해야 할 "최소 픽셀 수" (보통 2.0 ~ 20.0)
    // 값이 클수록 카메라가 조금만 멀어져도 빠르게 다음 LOD로 전환됨.
    float GridMinPixelsBetweenCells;

    // 가장 얇은 그리드 선 한 칸의 실제 월드 크기 (기본 1.0m)
    float GridCellSize;

    // 얇은 선의 색상과 투명도 (RGBA)
    float4 GridColorThin;

    // 두꺼운 선의 색상과 투명도 (RGBA)
    float4 GridColorThick;
}

// ----------------------------------------------------------------
// [정점 데이터: 버퍼 없이 셰이더 내부에서 하드코딩으로 생성 (절차적 생성)]
// ----------------------------------------------------------------
// Z-up 좌표계를 기준으로 XY 평면(바닥)에 그려지는 1x1 크기의 기본 Quad
static const float3 Pos[4] = {
    float3(-1.0, -1.0, 0.0), // 좌하단
    float3( 1.0, -1.0, 0.0), // 우하단
    float3( 1.0,  1.0, 0.0), // 우상단
    float3(-1.0,  1.0, 0.0), // 좌상단
};

// 사각형을 그리기 위한 두 개의 삼각형 인덱스 순서 (0-2-1, 2-0-3)
static const int Indices[6] = {
    0, 2, 1,
    2, 0, 3,
};

// 정점 셰이더 입력
struct VertexInput
{
    // 렌더링 파이프라인이 자동으로 채워주는 현재 정점의 번호 (0 ~ 5)
    uint vertex_id : SV_VertexID;
};

// 정점 셰이더 출력 (픽셀 셰이더로 넘어갈 데이터)
struct VertexOutput
{
    float4 position   : SV_Position; // 화면(Clip Space)에 투영된 최종 좌표
    float4 world_pos  : Color;       // 픽셀 셰이더에서 패턴을 그릴 기준 월드 좌표
    float2 camera_pos : TEXCOORD0;   // 픽셀 셰이더의 Falloff 계산을 위한 카메라 XY 좌표
    float  grid_size  : TEXCOORD1;   // 픽셀 셰이더의 Falloff 계산을 위한 그리드 크기
};

// ================================================================
// 정점 셰이더 (Vertex Shader)
// ================================================================
VertexOutput VSMain(VertexInput input)
{
    VertexOutput output;

    // 1. 하드코딩된 정점 배열에서 로컬 위치를 가져와 GridSize만큼 스케일을 곱함
    int index = Indices[input.vertex_id];
    float3 world_pos = Pos[index] * GridSize;

    // 2. 바닥 평면이 항상 카메라를 따라다니도록 카메라의 XY 위치로 이동
    world_pos.xy += CameraPos.xy;

    // 3. VP 행렬을 사용하여 최종 위치 계산 (월드 좌표 -> 화면 클립 좌표)
    output.position = mul(VP, float4(world_pos, 1.0f));
    output.world_pos = float4(world_pos, 1.0f);

    // 4. PS에서 필요한 데이터를 전달
    output.camera_pos = CameraPos.xy;
    output.grid_size = GridSize;

    return output;
}

/**
 * 특정 간격(cell_size)을 가진 그리드 선의 투명도(Alpha)를 계산합니다.
 * @param world_pos 현재 픽셀의 월드 좌표
 * @param cell_size 그리고자 하는 그리드 한 칸의 크기 (예: 1.0m, 10.0m)
 * @param derivative 화면 픽셀 변화량(fwidth 결과값). 에일리어싱을 막고 선의 두께를 일정하게 유지하는 데 사용.
 * @return float 계산된 선의 알파값 (0.0: 완전 투명 ~ 1.0: 선명한 선)
 */
float DrawGrid(float2 world_pos, float cell_size, float2 derivative)
{
    // 1. 스케일 및 픽셀 변화량 정규화
    float2 coord = world_pos / cell_size;
    float2 dudv = derivative / cell_size;

    // 2. 선의 중심축 거리 계산 (Triangle Wave 공식)
    // [fmod 대신 frac을 사용하는 이유]
    // fmod는 음수 좌표(-X, -Y)에서 거울처럼 반사되어 그리드가 깨지지만, frac은 음수에서도 무한히 이어지는 패턴(0.0~1.0)을 보장합니다.
    // abs(frac(x - 0.5) - 0.5) 연산으로 선이 픽셀 한쪽으로 쏠리지 않고 정확히 중앙에 정렬되도록 만듭니다.
    float2 grid_dist = abs(frac(coord - 0.5f) - 0.5f) / dudv;

    // 3. 거리를 두께로 변환 및 교차점 중첩 방지
    // min(grid_dist.x, y)를 통해 가로선과 세로선 중 더 거리가 가까운(더 진한) 선만 선택합니다.
    // 1.0f에서 거리를 빼주어 '선 굵기'로 만들고, 음수 값은 saturate로 0으로 잘라냅니다.
    float line_alpha = saturate(1.0f - min(grid_dist.x, grid_dist.y));

    return line_alpha;
}

// ================================================================
// 프래그먼트/픽셀 셰이더 (Fragment/Pixel Shader)
// ================================================================
float4 PSMain(VertexOutput input) : SV_Target0
{
    // ------------------------------------------------------------
    // [1단계: 화면 픽셀 변화량 측정 (에일리어싱 방지)]
    // ------------------------------------------------------------

    // [기존 방식] ddx, ddy를 각각 구해서 피타고라스 정리(length)로 길이를 구하던 방식
    // float2 dvx = float2(ddx(input.world_pos.x), ddy(input.world_pos.x));
    // float2 dvy = float2(ddx(input.world_pos.y), ddy(input.world_pos.y));

    // float lx = length(dvx);
    // float ly = length(dvy);

    // float2 derivative = float2(lx, ly);

    // [최적화 방식] HLSL 내장 함수 fwidth로 대체
    float2 derivative = fwidth(input.world_pos.xy);
    float linear_dist = length(derivative); // 방향에 상관없는 절대적인 변화량 길이

    // ------------------------------------------------------------
    // [2단계: 무한 로그 LOD 레벨 계산]
    // ------------------------------------------------------------

    // 거리가 멀어져 선이 너무 촘촘해지면(linear_dist 증가) LOD 레벨이 올라갑니다.
    // log10을 사용하여 거리에 따라 LOD 값이 0.1, 0.5, 1.2 등 소수로 부드럽게 증가합니다.
    float lod = max(0.0f, log10(linear_dist * GridMinPixelsBetweenCells / GridCellSize));

    // ------------------------------------------------------------
    // [3단계: 카메라 거리에 맞는 3단계 그리드 스케일(10배수) 계산]
    // ------------------------------------------------------------

    // floor(lod)를 통해 현재 보여야 할 가장 얇은 선의 기준 스케일을 10배수 단위로 구합니다.
    // 예) LOD가 1.5면 10^1 = 10m 단위부터 그리기 시작함.
    float grid_cell_size_lod0 = GridCellSize * pow(10.0f, floor(lod));
    float grid_cell_size_lod1 = grid_cell_size_lod0 * 10.0f;
    float grid_cell_size_lod2 = grid_cell_size_lod1 * 10.0f;

    // 각 LOD 단계별 그리드 선의 투명도(Alpha) 추출
    float grid_lod0_alpha = DrawGrid(input.world_pos.xy, grid_cell_size_lod0, derivative);
    float grid_lod1_alpha = DrawGrid(input.world_pos.xy, grid_cell_size_lod1, derivative);
    float grid_lod2_alpha = DrawGrid(input.world_pos.xy, grid_cell_size_lod2, derivative);

    // ------------------------------------------------------------
    // [4단계: 부드러운 LOD 전환 (Crossfade) 로직]
    // ------------------------------------------------------------

    // if/else 로 선을 교체하면 뚝뚝 끊기는 Popping 현상이 발생하므로,
    // lerp를 사용하여 거리에 따라 서서히 선을 투명하게 지웁니다.
    float lod_fade = frac(lod); // LOD 전환 진행률 (0.0 ~ 1.0)

    // 가장 얇은 선(LOD 0)은 다음 단계로 갈수록 서서히 깎여서 0.0(투명)이 됨
    float lod0_faded = lerp(grid_lod0_alpha, 0.0f, lod_fade);
    float lod1_faded = grid_lod1_alpha;
    float lod2_faded = grid_lod2_alpha;

    // ------------------------------------------------------------
    // [5단계: 선 굵기에 따른 색상 결정 (Color Mix)]
    // ------------------------------------------------------------

    // 모든 선을 교차점 중첩 없이 하나로 병합
    float final_alpha = max(max(lod0_faded, lod1_faded), lod2_faded);

    // 이 픽셀이 '두꺼운 선 색상(Thick)'을 얼마나 가져야 하는지 가중치 계산
    // - LOD 2 영역: grid_lod2_alpha (최대 1.0)
    // - LOD 1 영역: grid_lod1_alpha에 (1.0 - lod_fade)를 곱해 거리에 따라 가중치를 서서히 줄임
    float thick_weight = max(grid_lod2_alpha, grid_lod1_alpha * (1.0f - lod_fade));

    // 계산된 가중치를 바탕으로 얇은 선 색상과 두꺼운 선 색상을 부드럽게 혼합(lerp)
    float4 base_color = lerp(GridColorThin, GridColorThick, thick_weight);

    // 결정된 색상에 계산된 최종 그리드 투명도 적용
    base_color.a *= final_alpha;

    // ------------------------------------------------------------
    // [6단계: 그리드 가장자리 페이드아웃 (Opacity Falloff)]
    // ------------------------------------------------------------

    // 카메라 위치를 기준으로 거리를 계산하여, 평면(GridSize) 끝으로 갈수록 투명하게 페이드아웃 처리
    float distance_from_camera = length(input.world_pos.xy - input.camera_pos);
    float opacity_falloff = 1.0f - saturate(distance_from_camera / input.grid_size);

    base_color.a *= opacity_falloff;

    return base_color;
}
