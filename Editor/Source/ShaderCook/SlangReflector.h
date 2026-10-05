#pragma once

#include "SimpleEditor/ShaderCook/ShaderCompiler.h"

#include <slang.h>


namespace se::editor
{
/**
 * Slang 링크 결과의 리플렉션을 엔진 계약과 검증용 원자료로 옮기는 변환기
 *
 * 리소스, 정점 입력, varying은 SPIR-V 레이아웃에서, dxil_uniform_buffers는 DXIL 레이아웃에서 읽습니다.
 * 리소스 개수와 슬롯은 코드에 남은 것이 아니라 선언 기준으로 계산합니다.
 */
class SlangReflector
{
public:
    SlangReflector(slang::ProgramLayout* spirv_layout, slang::ProgramLayout* dxil_layout);

    /** 스테이지 필터 전 선언된 리소스 전체 */
    [[nodiscard]] const Array<ShaderBindingRecord>& GetDeclaredBindings() const { return declared_bindings; }

    /** entry_index 진입점의 SDL 계약을 만듭니다. */
    [[nodiscard]] ShaderStageInterface ReflectInterface(SlangUInt entry_index, EShaderStage stage) const;

    /** stage가 쓰는 상수 버퍼를 DXIL 레이아웃으로 읽습니다. */
    [[nodiscard]] Array<ShaderUniformBuffer> ReflectDxilUniformBuffers(EShaderStage stage) const;

    /** entry_index 진입점의 시스템 값이 아닌 입력 */
    [[nodiscard]] Array<ShaderVarying> ReflectInputs(SlangUInt entry_index) const;

    /** entry_index 진입점의 시스템 값이 아닌 출력 */
    [[nodiscard]] Array<ShaderVarying> ReflectOutputs(SlangUInt entry_index) const;

private:
    slang::ProgramLayout* spirv_layout;
    slang::ProgramLayout* dxil_layout;
    Array<ShaderBindingRecord> declared_bindings;
};
} // namespace se::editor
