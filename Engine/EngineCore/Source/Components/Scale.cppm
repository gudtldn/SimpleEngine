export module SimpleEngine.Components:Scale;
import SimpleEngine.Math;


export struct Scale
{
    Vector3 value;

public:
    constexpr Scale() = default;

    constexpr explicit Scale(const Vector3& in_value)
        : value(in_value)
    {
    }
};
