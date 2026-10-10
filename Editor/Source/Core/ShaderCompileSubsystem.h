#pragma once

#include "SimpleEditor/ShaderCook/ShaderCompiler.h"
#include "SimpleEditor/ShaderCook/ShaderCooker.h"

#include "SimpleEngine/Core/Reflection/ReflectMacros.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Core/Reflection/Rtti.h"
#include "SimpleEngine/Core/Subsystem/IUpdatable.h"
#include "SimpleEngine/Core/Subsystem/SubsystemBase.h"

#include <memory>


namespace se::editor
{
/**
 * 셰이더 쿡 및 핫 리로드를 담당하는 Subsystem
 * 시작할 때 셰이더를 DDC에 쿡하고, F5를 누르면 바뀐 셰이더만 다시 쿡해 파이프라인을 비웁니다.
 */
class ShaderCompileSubsystem : public se::SubsystemBase, public se::IUpdatable
{
    friend struct ::se::Registrar<ShaderCompileSubsystem>;

public:
    SE_RTTI(ShaderCompileSubsystem)

    //~ Begin SubsystemBase
    [[nodiscard]] virtual bool Initialize() override;
    virtual void Release() override;
    //~ End SubsystemBase

    //~ Begin IUpdatable
    virtual void Update(f64 delta_time) override;
    //~ End IUpdatable

private:
    /** 셰이더 폴더를 모두 쿡하고, 새로 쿡한 셰이더 개수를 반환합니다. 최신인 셰이더는 건너뜁니다. */
    u32 CookAll() const; // NOLINT(*-use-nodiscard)

    /** 바뀐 셰이더를 다시 쿡하고, 하나라도 쿡했으면 셰이더와 파이프라인 캐시를 비웁니다. */
    void RecookChanged() const;

private:
    SE_ANNOTATE(compiler, Ignore)
    std::unique_ptr<ShaderCompiler> compiler;
    SE_ANNOTATE(cooker, Ignore)
    std::unique_ptr<ShaderCooker> cooker;
};
} // namespace se::editor

SE_DECLARE_REFLECTION(se::editor::ShaderCompileSubsystem)
