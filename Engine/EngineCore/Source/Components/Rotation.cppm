export module SimpleEngine.Components:Rotation;
import SimpleEngine.Math;


export struct Rotation
{
    Rotator value;

public:
    constexpr Rotation() = default;

    constexpr explicit Rotation(const Rotator& in_value)
        : value(in_value)
    {
    }
};
