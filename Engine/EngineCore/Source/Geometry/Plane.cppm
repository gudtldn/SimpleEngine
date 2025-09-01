export module SE.Geometry:Plane;
import :Vertex;

import SE.Types;


export
{
    constexpr Vertex plane_vertices[] =
    {
        { { -1.000000f, -1.000000f, 0.000000f }, { 0.717647f, 0.286275f, 0.847059f, 1.000000f } },
        { { 1.000000f, -1.000000f, 0.000000f }, { 0.082353f, 0.858824f, 0.317647f, 1.000000f } },
        { { -1.000000f, 1.000000f, 0.000000f }, { 0.968627f, 0.678431f, 0.705882f, 1.000000f } },
        { { 1.000000f, 1.000000f, 0.000000f }, { 0.270588f, 0.015686f, 0.478431f, 1.000000f } }
    };

    constexpr uint16 plane_indices[] =
    {
        1, 2, 0,
        1, 3, 2
    };
}
