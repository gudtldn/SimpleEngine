module;
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
export module SimpleEngine.Config;

import SimpleEngine.Logging;
import SimpleEngine.Platform.Types;
import std;


namespace se::config
{
export class Config;

/**
 * TOML 파일 파싱 결과를 나타내는 타입입니다.
 * @details 성공 시 Config 객체를, 실패 시 toml::parse_error를 반환합니다.
 */
export using ParseResult = std::expected<Config, toml::parse_error>;

/**
 * TOML 파일을 읽고 쓰?는 설정 관리 클래스입니다.
 * @todo 파일 쓰기 구현하기
 */
class Config
{
public:
    /**
     * 지정된 경로의 TOML을 읽어 Config 객체를 생성합니다.
     * @param config_file_path TOML 파일 경로
     * @return 정상적으로 읽어오면 Config 객체를, 실패하면 toml::parse_error를 반환합니다.
     */
    static ParseResult ReadConfig(const std::filesystem::path& config_file_path);

    /**
     * 읽어온 TOML에서 지정된 키에 해당하는 값을 가져옵니다.
     * @tparam To 반환받을 값의 타입
     * @param key 값을 가져올 Key
     * @return Key가 존재하고, To로 변환이 가능하다면 값을 반환하고, 없다면 std::nullopt를 반환합니다.
     */
    template <typename To>
    std::optional<To> GetValue(std::u8string_view key) const;

    /**
     * 읽어온 TOML에서 지정된 키에 해당하는 값을 가져오거나, 없을 경우 기본값을 반환합니다.
     * @tparam To 반환받을 값의 타입
     * @tparam U 기본값의 타입. To 타입으로 변환 가능해야 합니다.
     * @param key 값을 가져올 Key
     * @param default_val Key가 존재하고, To로 변환이 가능하다면 값을 반환하고, 없다면 default_val을 반환합니다.
     * @return
     */
    template <typename To, typename U = To>
        requires std::is_convertible_v<U, To>
    To GetValueOr(std::u8string_view key, U&& default_val = U{}) const;

    /**
     * 읽어온 TOML에서 지정된 키에 해당하는 배열을 가져옵니다.
     * @tparam ElementType 배열 요소의 타입
     * @param key_path 값을 가져올 Key
     * @return Key가 존재하고, 정상적으로 변환이 가능하다면 값을 반환하고, 없다면 std::nullopt를 반환합니다.
     * @warning 배열 내 요소들의 타입이 일치하지 않거나 ElementType으로 변환 불가능하면 std::nullopt를 반환합니다.
     */
    template <typename ElementType>
    std::optional<std::vector<ElementType>> GetArray(std::u8string_view key_path) const;

    /**
     * 읽어온 TOML에서 지정된 키에 해당하는 Table을 Config 객체로 반환합니다.
     * @param key_path Table을 가져올 키 경로
     * @return 키 경로가 존재하고 Table으로 변환 가능할 경우 Config 객체를 반환하며,
     *         그렇지 않을 경우 std::nullopt를 반환합니다.
     */
    std::optional<Config> GetTable(std::u8string_view key_path) const;

private:
    explicit Config(toml::table&& table);

    /** TOML내의 노드를 가져옵니다. */
    toml::node_view<const toml::node> FindNode(std::u8string_view path_str) const;

private:
    toml::table config_table;
};

inline Config::Config(toml::table&& table)
    : config_table(std::move(table))
{
}

template <typename To>
std::optional<To> Config::GetValue(std::u8string_view key) const
{
    return FindNode(key).value<To>();
}

template <typename To, typename U>
    requires std::is_convertible_v<U, To>
To Config::GetValueOr(std::u8string_view key, U&& default_val) const
{
    if (auto val = GetValue<To>(key))
    {
        return *val;
    }
    return static_cast<To>(std::forward<U>(default_val));
}

template <typename ElementType>
std::optional<std::vector<ElementType>> Config::GetArray(std::u8string_view key_path) const
{
    if (const auto node_view = FindNode(key_path))
    {
        if (auto* arr = node_view.as_array())
        {
            std::vector<ElementType> result_vec;
            result_vec.reserve(arr->size());
            bool success = true;
            for (const auto& elem_node : *arr)
            {
                if (auto val_opt = elem_node.value<ElementType>())
                {
                    result_vec.push_back(*val_opt);
                }
                else
                {
                    ConsoleLog(ELogLevel::Warning, u8"Failed to get array element: {}", elem_node.as_string()->get());

                    success = false; // 배열 내 타입 불일치
                    break;
                }
            }
            if (success)
            {
                return result_vec;
            }
        }
    }
    return std::nullopt;
}
}
