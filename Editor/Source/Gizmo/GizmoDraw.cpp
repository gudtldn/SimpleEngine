#include "SimpleEditor/Gizmo/GizmoDraw.h"

#include "SimpleEditor/Gizmo/GizmoDrawList.h"
#include "SimpleEditor/Gizmo/GizmoSubsystem.h"
#include "SimpleEngine/Utility/SubsystemUtils.h"


namespace se::editor
{
using namespace se::math;

namespace
{
// Vector3(double) -> Vector3f(float) 축소 변환
Vector3f ToVector3f(const Vector3& v)
{
    return { static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z) };
}

GizmoDrawList* GetDrawListOrNull()
{
    if (GizmoSubsystem* subsystem = se::GetSubsystem<GizmoSubsystem>())
    {
        return &subsystem->GetDrawList();
    }
    return nullptr;
}
} // namespace

void DrawGizmoLine(const Vector3& start, const Vector3& end, const LinearColor& color)
{
    GizmoDrawList* list = GetDrawListOrNull();
    if (!list) { return; }

    list->AddLine(
        { .position = ToVector3f(start), .color = color },
        { .position = ToVector3f(end),   .color = color }
    );
}

void DrawGizmoTriangle(const Vector3& a, const Vector3& b, const Vector3& c, const LinearColor& color)
{
    GizmoDrawList* list = GetDrawListOrNull();
    if (!list) { return; }

    list->AddTriangle(
        { .position = ToVector3f(a), .color = color },
        { .position = ToVector3f(b), .color = color },
        { .position = ToVector3f(c), .color = color }
    );
}

void DrawGizmoArrow(const Vector3& from, const Vector3& to, float head_size, const LinearColor& color, int32 segments)
{
    GizmoDrawList* list = GetDrawListOrNull();
    if (!list) { return; }

    // 1. 화살표 몸통 라인
    list->AddLine(
        { .position = ToVector3f(from), .color = color },
        { .position = ToVector3f(to),   .color = color }
    );

    // 2. 화살촉(원뿔) 기하학 생성
    const Vector3 direction = (to - from);
    const double dir_length = direction.Length();
    if (dir_length < KINDA_SMALL_NUMBER) { return; }

    const Vector3 dir_normalized = direction / dir_length;

    // 수직 벡터 계산 (방향과 직교하는 두 축)
    Vector3 tangent, bitangent;
    dir_normalized.GetOrthogonalAxes(tangent, bitangent);

    // 원뿔 밑면의 중심점: 끝점에서 머리 길이(head_size * 2.0)만큼 뒤로 이동
    const double head_length = head_size * 2.0;
    const Vector3 cone_base_center = to - dir_normalized * head_length;

    // 원뿔의 꼭짓점(to)과 밑면의 둘레 점들을 이어 삼각형 팬(Triangle Fan) 구성
    const Radian angle_step = Radian{ PI_DOUBLE * 2.0 } / segments;
    for (int32 i = 0; i < segments; ++i)
    {
        const Radian angle0 = angle_step * i;
        const Radian angle1 = angle_step * (i + 1);

        const Vector3 p0 = cone_base_center + (tangent * Cos(angle0) + bitangent * Sin(angle0)) * head_size;
        const Vector3 p1 = cone_base_center + (tangent * Cos(angle1) + bitangent * Sin(angle1)) * head_size;

        list->AddTriangle(
            { .position = ToVector3f(to), .color = color },
            { .position = ToVector3f(p0), .color = color },
            { .position = ToVector3f(p1), .color = color }
        );
    }
}

void DrawGizmoCircle(const Vector3& center, const Vector3& normal, float radius, const LinearColor& color, int32 segments)
{
    GizmoDrawList* list = GetDrawListOrNull();
    if (!list) { return; }

    const Vector3 n = normal.GetNormalized();
    Vector3 tangent, bitangent;
    n.GetOrthogonalAxes(tangent, bitangent);

    const Radian angle_step = Radian{ PI_DOUBLE * 2.0 } / segments;
    for (int32 i = 0; i < segments; ++i)
    {
        const Radian angle0 = angle_step * i;
        const Radian angle1 = angle_step * (i + 1);

        const Vector3 p0 = center + (tangent * Cos(angle0) + bitangent * Sin(angle0)) * radius;
        const Vector3 p1 = center + (tangent * Cos(angle1) + bitangent * Sin(angle1)) * radius;

        list->AddLine(
            { .position = ToVector3f(p0), .color = color },
            { .position = ToVector3f(p1), .color = color }
        );
    }
}

void DrawGizmoWorldAxes(const Vector3& origin, float length)
{
    DrawGizmoLine(origin, origin + Vector3::Right()   * length, LinearColor::Red());
    DrawGizmoLine(origin, origin + Vector3::Forward() * length, LinearColor::Green());
    DrawGizmoLine(origin, origin + Vector3::Up()      * length, LinearColor::Blue());
}
} // namespace se::editor
