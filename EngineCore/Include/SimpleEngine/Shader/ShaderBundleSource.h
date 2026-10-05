#pragma once

#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Types/Guid.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Shader/ShaderBundle.h"


namespace se
{
// forward declaration
class DerivedDataCache;

/**
 * 프로그램 VPath로 쿡된 셰이더 번들을 주는 곳
 * 번들을 어디에 두는지는 구현이 정합니다.
 */
class SE_CORE_API IShaderBundleSource
{
public:
    virtual ~IShaderBundleSource() = default;

    /**
     * program의 번들을 읽습니다.
     * @return 번들이 없거나, 형식 버전이 다르거나, 손상되었으면 이유를 담은 에러
     */
    [[nodiscard]] virtual Expected<ShaderBundle, String> Load(const VPath& program) const = 0;
};

/** 에디터가 쿡해 DDC에 저장한 번들을 읽는 소스 */
class SE_CORE_API DdcShaderBundleSource final : public IShaderBundleSource
{
public:
    explicit DdcShaderBundleSource(const DerivedDataCache& ddc);

    [[nodiscard]] virtual Expected<ShaderBundle, String> Load(const VPath& program) const override;

    /** program의 번들을 저장하는 DDC 키 */
    [[nodiscard]] static Guid KeyOf(const VPath& program);

private:
    const DerivedDataCache& ddc;
};
} // namespace se
