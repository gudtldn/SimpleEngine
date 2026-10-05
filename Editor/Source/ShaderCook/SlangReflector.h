#pragma once

#include "SimpleEditor/ShaderCook/ShaderCompiler.h"

#include <slang.h>


namespace se::editor
{
/**
 * Slang 리플렉션을 엔진의 셰이더 인터페이스 타입으로 변환합니다.
 *
 * 리소스, 정점 입력, 스테이지 입출력은 SPIR-V 레이아웃에서 읽고, dxil_uniform_buffers만 DXIL 레이아웃에서 읽습니다.
 * Slang은 쓰지 않는 리소스를 코드에서 지우므로, 리소스 개수와 슬롯은 소스에 선언된 것을 기준으로 계산합니다.
 */
class SlangReflector
{
public:
    SlangReflector(slang::ProgramLayout* spirv_layout, slang::ProgramLayout* dxil_layout);

    /** 모든 스테이지에 선언된 리소스 (스테이지별로 거르기 전) */
    [[nodiscard]] const Array<ShaderBindingRecord>& GetDeclaredBindings() const { return declared_bindings; }

    /** entry_index 진입점의 셰이더 인터페이스를 만듭니다. */
    [[nodiscard]] ShaderStageInterface ReflectInterface(SlangUInt entry_index, EShaderStage stage) const;

    /** stage가 쓰는 상수 버퍼를 DXIL 레이아웃으로 읽습니다. */
    [[nodiscard]] Array<ShaderUniformBuffer> ReflectDxilUniformBuffers(EShaderStage stage) const;

    /** entry_index 진입점의 입력 중 시스템 값을 제외한 것 */
    [[nodiscard]] Array<ShaderVarying> ReflectInputs(SlangUInt entry_index) const;

    /** entry_index 진입점의 출력 중 시스템 값을 제외한 것 */
    [[nodiscard]] Array<ShaderVarying> ReflectOutputs(SlangUInt entry_index) const;

private:
    slang::ProgramLayout* spirv_layout;
    slang::ProgramLayout* dxil_layout;
    Array<ShaderBindingRecord> declared_bindings;
};
} // namespace se::editor
