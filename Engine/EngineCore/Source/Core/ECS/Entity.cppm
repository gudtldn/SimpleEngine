export module SimpleEngine.Core:ECS.Entity;

import SimpleEngine.Types;
import std;


export struct Entity
{
    uint32 id;
    uint32 generation;

    [[nodiscard]] bool operator==(const Entity& other) const = default;
    [[nodiscard]] bool operator!=(const Entity& other) const = default;
};
