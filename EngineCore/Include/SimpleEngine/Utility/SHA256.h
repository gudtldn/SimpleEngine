#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/HAL/PlatformTypes.h"
#include "SimpleEngine/Core/Types/HashDigest.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se
{
/**
 * SHA-256 해시 계산 유틸리티
 *
 * 에셋의 소스 파일 변경 감지를 위해 파일 또는 바이트 데이터의
 * SHA-256 해시를 계산합니다.
 */
struct SE_CORE_API SHA256
{
    SHA256() = delete;

    /**
     * 파일의 SHA-256 해시를 계산합니다.
     * @param file_path 해시를 계산할 파일의 경로
     * @return SHA-256 digest. 실패 시 zero digest.
     */
    [[nodiscard]] static ContentHash HashFile(const Path& file_path);

    /**
     * 바이트 데이터의 SHA-256 해시를 계산합니다.
     * @param data 해시를 계산할 바이트 배열
     * @return SHA-256 digest
     */
    [[nodiscard]] static ContentHash HashBytes(ArrayView<const u8> data);

    /**
     * 문자열 데이터의 SHA-256 해시를 계산합니다.
     * @param str 해시를 계산할 문자열
     * @return SHA-256 digest
     */
    [[nodiscard]] static ContentHash HashString(StringView str);
};
} // namespace se
