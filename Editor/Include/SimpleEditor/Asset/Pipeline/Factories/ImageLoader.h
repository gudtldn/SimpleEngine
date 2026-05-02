#pragma once

#include "SimpleEditor/EditorCommon.h"

#include "SimpleEngine/Asset/Types/Texture2D.h"
#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Error/IError.h"
#include "SimpleEngine/Core/Types/Path.h"


namespace se::editor
{
/** 이미지 로드 중 발생하는 에러 */
class SE_EDITOR_API ImageLoadError final : public IError
{
public:
    enum class EType
    {
        InvalidSource,    // 소스가 null이거나 유효하지 않음
        LoadFailed,       // SDL3_image 로드 실패
        ConversionFailed, // RGBA8 변환 실패
        IOError,          // SDL_IO 생성 실패 등 입출력 에러
        Unknown           // 알 수 없는 에러
    };
    using enum EType;

public:
    ImageLoadError(EType type, String message)
        : type(type), message(std::move(message)) {}

    [[nodiscard]] virtual const char* What() const noexcept override { return message.CStr(); }
    [[nodiscard]] virtual const IError* Source() const noexcept override { return nullptr; }

    [[nodiscard]] EType GetType() const noexcept { return type; }

private:
    EType type;
    String message;
};

/** SDL3_image 디코딩 결과 (RGBA8, row-tightly-packed) */
struct ImageData
{
    Array<uint8> pixels;
    uint32 width = 0;
    uint32 height = 0;
    ETextureFormat format = ETextureFormat::None; // UNORM or UNORM_SRGB
};

/** 이미지 로드 결과 타입 */
using ImageLoadResult = Expected<ImageData, ImageLoadError>;

/**
 * SDL3_image를 사용하여 다양한 소스에서 RGBA8 픽셀을 로드하는 유틸리티 클래스
 */
class SE_EDITOR_API ImageLoader
{
public:
    ImageLoader() = delete;

    /**
     * 외부 텍스처 파일에서 이미지를 로드합니다.
     * @param file_path 이미지 파일 경로
     * @param is_srgb sRGB 감마 인코딩 여부 (포맷 결정에 사용)
     */
    [[nodiscard]] static ImageLoadResult LoadFromFile(const Path& file_path, bool is_srgb);

    /**
     * 메모리 상의 압축 이미지 바이트(PNG/JPG/TGA 등)에서 이미지를 로드합니다.
     * @param data 압축 이미지 데이터 뷰
     * @param is_srgb sRGB 감마 인코딩 여부
     * @param format_hint Assimp achFormatHint ("png", "jpg", ...) 또는 빈 문자열
     */
    [[nodiscard]] static ImageLoadResult LoadFromMemory(ArrayView<const uint8> data, bool is_srgb, StringView format_hint);

    /**
     * 이미 디코딩된 RGBA8 raw pixels를 ImageData로 패키징합니다.
     * Assimp 임베디드 텍스처가 이미 RGBA8로 디코딩된 경우에 사용합니다.
     * @param rgba8_pixels RGBA8 픽셀 데이터 뷰 (row-tightly-packed)
     * @param width 이미지 너비
     * @param height 이미지 높이
     * @param is_srgb sRGB 감마 인코딩 여부
     */
    [[nodiscard]] static ImageData LoadFromRawPixels(ArrayView<const uint8> rgba8_pixels, uint32 width, uint32 height, bool is_srgb);
};
} // namespace se::editor
