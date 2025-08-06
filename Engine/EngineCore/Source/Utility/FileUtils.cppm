export module SimpleEngine.Utility:FileUtils;

import SimpleEngine.Types;
import std;


export namespace se::utility::file_utils
{
template <typename T>
using FileResult = std::expected<T, std::u8string>;

/** binary파일을 읽어 byte array로 반환합니다. */
[[nodiscard]] FileResult<std::vector<uint8>> ReadFromBinary(const std::filesystem::path& file_path);
}
