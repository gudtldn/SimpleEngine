export module SE.Subsystems.AssetSubsystem;

import SE.Interface.ISubsystem;
import SE.Types;
import std;


/**
 *
 */
export class AssetSubsystem : public ISubsystem<>
{
public:
    //~ Begin ISubsystem
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End ISubsystem
};
