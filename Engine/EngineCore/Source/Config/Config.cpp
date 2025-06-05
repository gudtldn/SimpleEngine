module;
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
module SimpleEngine.Config;


namespace se::config
{
ParseResult Config::ReadConfig(const std::filesystem::path& config_file_path)
{
    toml::parse_result result = toml::parse_file(config_file_path.generic_u8string());
    if (result.failed())
    {
        return std::unexpected{std::move(result).error()};
    }
    return Config{std::move(result).table()};
}

std::optional<Config> Config::GetTable(std::u8string_view key_path) const
{
    if (const auto node_view = FindNode(key_path))
    {
        if (auto* sub_table = node_view.as_table())
        {
            // toml::table을 복사하여 새로운 Config 객체 생성
            return Config(toml::table(*sub_table));
        }
    }
    return std::nullopt;
}

bool Config::WriteConfig(const std::filesystem::path& config_file_path) const
{
    std::ofstream file_stream(config_file_path.generic_string(), std::ios::binary | std::ios::trunc);
    if (!file_stream.is_open())
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to open config file for writing: {}", config_file_path.generic_u8string());
        return false;
    }

    // config를 직렬화
    file_stream << config_table;

    if (file_stream.fail())
    {
        ConsoleLog(ELogLevel::Error, u8"Failed to write config file: {}", config_file_path.generic_u8string());
        file_stream.close();
        return false;
    }

    file_stream.close();
    if (file_stream.fail())
    {
        ConsoleLog(ELogLevel::Warning, u8"Potential issue closing file stream for: {}", config_file_path.generic_u8string());
    }

    return true;
}

toml::node_view<const toml::node> Config::FindNode(std::u8string_view path_str) const
{
    const std::string key_str = std::string(path_str.begin(), path_str.end());
    return config_table.at_path(key_str);
}
}
