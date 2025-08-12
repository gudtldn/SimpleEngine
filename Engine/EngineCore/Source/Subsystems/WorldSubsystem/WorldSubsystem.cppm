export module SimpleEngine.Subsystems.WorldSubsystem;

import SimpleEngine.Interface.ISubsystem;
import SimpleEngine.Core;
import std;

using namespace se::core::ecs;


/**
 *
 */
export class WorldSubsystem : public ISubsystem<>
{
public:
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;

private:
    // std::vector<std::unique_ptr<World>> world_list;
};
