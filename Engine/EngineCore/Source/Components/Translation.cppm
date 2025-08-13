export module SimpleEngine.Components:Translation;
import SimpleEngine.Math;


export struct Translation
{
    Vector3 value;

public:
    constexpr Translation() = default;

    constexpr explicit Translation(const Vector3& in_value)
        : value(in_value)
    {
    }
};
