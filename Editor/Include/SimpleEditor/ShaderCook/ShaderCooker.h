#pragma once

#include "SimpleEditor/EditorCommon.h"
#include "SimpleEditor/ShaderCook/ShaderCookError.h"

#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Shader/ShaderBundle.h"


namespace se
{
class DerivedDataCache;
} // namespace se

namespace se::editor
{
class ShaderCompiler;

/** CookDirectory가 처리한 셰이더 개수 */
struct ShaderCookSummary
{
    u32 cooked = 0;
    u32 up_to_date = 0;
    u32 failed = 0;
};

/**
 * 셰이더 소스를 컴파일하고 검증해 번들로 만든 뒤 DDC에 저장하는 쿠커
 */
class SE_EDITOR_API ShaderCooker
{
public:
    explicit ShaderCooker(const ShaderCompiler& compiler);

    /**
     * shader_dir 바로 아래의 .hlsl을 모두 쿡해 ddc에 저장합니다.
     * DDC의 번들이 최신이면 건너뛰고, 실패한 셰이더는 로그를 남기고 다음 셰이더로 넘어갑니다.
     */
    ShaderCookSummary CookDirectory(const VPath& shader_dir, DerivedDataCache& ddc) const;

    /** DDC를 거치지 않고 소스 하나를 번들로 만듭니다. */
    [[nodiscard]] ShaderCookResult<ShaderBundle> CookFile(const VPath& shader_vpath) const;

private:
    /** ddc에 있는 번들이 지금 소스와 툴체인으로 만든 것인지 확인합니다. */
    [[nodiscard]] bool IsUpToDate(const Guid& key, const DerivedDataCache& ddc) const;

    /** 의존 파일의 경로, 내용과 툴체인을 합친 해시. 의존 파일이 없으면 NullOpt */
    [[nodiscard]] Optional<ContentHash> HashSources(const Array<String>& dependencies) const;

private:
    const ShaderCompiler& compiler;
};
} // namespace se::editor
