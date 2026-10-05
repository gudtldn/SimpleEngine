#pragma once

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Reflection/Registrar.h"
#include "SimpleEngine/Shader/ShaderInterface.h"


namespace se
{
/** 스테이지 하나를 포맷 하나로 컴파일한 바이트코드 */
struct ShaderBlob
{
    EShaderStage stage = EShaderStage::Vertex;
    EShaderFormat format = EShaderFormat::SPIRV;

    String entry_point;
    Array<u8> code;

    [[nodiscard]] bool operator==(const ShaderBlob&) const = default;
};

/**
 * 셰이더 프로그램 하나의 쿡 산출물
 *
 * 모든 포맷의 바이트코드와 엔진이 소유하는 인터페이스를 함께 담습니다.
 * 엔진 직렬화(BinaryFileWriter)로 쓰므로, 필드가 바뀌면 스키마 해시가 달라져 옛 번들은 읽기 단계에서 거절됩니다.
 */
struct SE_CORE_API ShaderBundle
{
    /** DDC cache_version으로 쓰는 번들 형식 버전 */
    static constexpr u32 FORMAT_VERSION = 1;

    ShaderProgramInterface program;
    Array<ShaderBlob> blobs;

    /** 소스 파일과 include한 파일의 VPath 문자열 */
    Array<String> dependencies;

    /** stage와 format이 모두 같은 블롭을 찾습니다. */
    [[nodiscard]] Optional<const ShaderBlob&> FindBlob(EShaderStage stage, EShaderFormat format) const;

    /** 번들 전체를 바이트로 씁니다. */
    [[nodiscard]] Array<u8> Serialize() const;

    /**
     * Serialize가 쓴 바이트를 읽습니다.
     * @return 헤더(루트 타입, 스키마 해시, 체크섬)가 맞지 않거나 내용이 손상되었으면 이유를 담은 에러
     */
    [[nodiscard]] static Expected<ShaderBundle, String> Deserialize(ArrayView<const u8> bytes);

    [[nodiscard]] bool operator==(const ShaderBundle&) const = default;
};
} // namespace se

SE_DECLARE_REFLECTION(se::ShaderBlob, SE_CORE_API)
SE_DECLARE_REFLECTION(se::ShaderBundle, SE_CORE_API)
