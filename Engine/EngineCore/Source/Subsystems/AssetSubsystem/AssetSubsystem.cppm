export module SE.Subsystems.AssetSubsystem;

import SE.Interface.ISubsystem;
import SE.Assets;
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

public:
    [[nodiscard]] se::assets::AssetManager* GetAssetManager() const noexcept { return asset_manager.get(); }

private:
    std::unique_ptr<se::assets::AssetManager> asset_manager;
};
