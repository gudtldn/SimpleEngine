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
 * TOML 파일을 읽고 쓰는 설정 관리 클래스입니다.
 */
class Config
{
public:
    Config() = default;
    ~Config() = default;

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
     * @param default_val Key가 존재하고, To로 변환이 가능하다면 값을 반환하고,
     *        없다면 default_val을 Config에 저장하고 반환합니다.
     * @return
     */
    template <typename To, typename U = To>
        requires std::is_convertible_v<U, To>
    To GetValueOrStore(std::u8string_view key, U&& default_val = U{});

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

    /**
     * @brief 지정된 키 경로에 값을 설정합니다. 중간 경로는 필요시 테이블로 자동 생성됩니다.
     * @tparam ValueType 설정할 값의 타입입니다.
     * @param[in] key_path 값을 설정할 키 경로입니다. 점(.)으로 구분된 중첩 경로를 지원합니다.
     * @param[in] value 설정할 값입니다. (Forwarding reference)
     * @return bool 설정에 성공하면 true, 실패하면 false (예: 경로 충돌).
     */
    template <typename ValueType>
    bool SetValue(std::u8string_view key_path, ValueType&& value);

    template <typename ElementType>
    bool SetValue(std::u8string_view key_path, const std::vector<ElementType>& vec_value);

    template <typename ElementType>
    bool SetValue(std::u8string_view key_path, std::vector<ElementType>&& vec_value);

    /**
     * @brief 현재 Config 객체의 내용을 지정된 파일 경로에 TOML 형식으로 저장합니다.
     * @param[in] config_file_path 저장할 파일의 경로입니다. 파일이 이미 존재하면 덮어씁니다.
     * @return bool 저장에 성공하면 true, 실패하면 false (예: 파일 열기/쓰기 오류).
     */
    bool WriteConfig(const std::filesystem::path& config_file_path) const;

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
To Config::GetValueOrStore(std::u8string_view key, U&& default_val)
{
    if (auto val = GetValue<To>(key))
    {
        return *val;
    }
    SetValue(key, std::forward<U>(default_val));
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
                    auto* str_node = elem_node.as_string();
                    ConsoleLog(ELogLevel::Warning, u8"Failed to get array element: {}", str_node ? str_node->get() : "null");

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

template <typename ValueType>
bool Config::SetValue(std::u8string_view key_path, ValueType&& value)
{
    if (key_path.empty())
    {
        ConsoleLog(ELogLevel::Error, u8"Config::SetValue: Key path cannot be empty.");
        return false;
    }

    const std::string path_std_string = std::string(key_path.begin(), key_path.end());
    std::string_view current_path_segment_view = path_std_string;

    toml::table* current_table_ptr = &config_table;
    size_t current_pos = 0;
    size_t next_dot_pos = current_path_segment_view.find('.');

    while (next_dot_pos != std::string_view::npos)
    {
        std::string segment_key_str(current_path_segment_view.substr(current_pos, next_dot_pos - current_pos));

        if (toml::node* node = current_table_ptr->get(segment_key_str)) // 노드가 이미 존재
        {
            if (node->is_table()) // 그리고 테이블임
            {
                current_table_ptr = node->as_table();
            }
            else // 테이블이 아닌 다른 타입의 노드임 -> 경로 충돌
            {
                ConsoleLog(
                    ELogLevel::Error,
                    u8"Config::SetValue: Path conflict at segment '{}' in path '{}'. Expected a table, but found a different type.",
                    segment_key_str, key_path
                );
                return false;
            }
        }
        else // 노드가 없음 -> 새로 테이블 생성
        {
            // emplace는 pair<iterator, bool>을 반환. iterator는 삽입된 요소, bool은 삽입 성공 여부.
            auto [it, success] = current_table_ptr->emplace(segment_key_str, toml::table{});
            if (!success || !it->second.is_table())
            {
                std::u8string u8_segment_key(segment_key_str.begin(), segment_key_str.end());
                ConsoleLog(
                    ELogLevel::Error,
                    u8"Config::SetValue: Failed to create intermediate table at segment '{}' in path '{}'.",
                    u8_segment_key, key_path
                );
                return false;
            }
            current_table_ptr = it->second.as_table();
        }
        current_pos = next_dot_pos + 1; // 다음 세그먼트 시작 위치
        next_dot_pos = current_path_segment_view.find('.', current_pos);
    }

    // 마지막 세그먼트 (실제 값을 설정할 키)
    std::string value_key_str(current_path_segment_view.substr(current_pos));
    if (value_key_str.empty())
    {
        // 경로가 "."으로 끝나는 경우 (예: "section.table.")
        ConsoleLog(
            ELogLevel::Error, u8"Config::SetValue: Key path '{}' is invalid as it ends with a delimiter without a final key name.",
            key_path
        );
        return false;
    }

    // 값 설정
    current_table_ptr->insert_or_assign(value_key_str, std::forward<ValueType>(value));

    return true;
}

template <typename ElementType>
bool Config::SetValue(std::u8string_view key_path, const std::vector<ElementType>& vec_value)
{
    return SetValue(key_path, std::vector<ElementType>(vec_value));
}

template <typename ElementType>
bool Config::SetValue(std::u8string_view key_path, std::vector<ElementType>&& vec_value)
{
    toml::array arr;
    arr.reserve(vec_value.size());
    for (auto&& elem : vec_value)
    {
        if constexpr (std::same_as<ElementType, bool>)
        {
            arr.push_back(static_cast<bool>(elem));
        }
        else
        {
            arr.push_back(std::move(elem));
        }
    }
    return SetValue(key_path, std::move(arr));
}
}
