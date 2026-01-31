#include "Utility/Config.h"

#include "Core/Types/VPath.h"
#include "Utility/FileUtils.h"


namespace se::utility
{
ParseResult Config::ReadConfig(const VPath& config_file_path)
{
    Optional physical_path_opt = config_file_path.Resolve();
    if (!physical_path_opt.HasValue())
    {
        // TODO: 에러 반환타입 리펙토링 toml::parse_result가 아닌, 새로운 타입으로
        return Unexpected{ toml::parse_file("").error() };
    }

    toml::parse_result result = toml::parse_file(physical_path_opt->ToString().CStr());
    if (result.failed())
    {
        return Unexpected{ std::move(result).error() };
    }
    return Config{ std::move(result).table() };
}

Optional<Config> Config::GetTable(std::string_view key_path) const
{
    if (const auto node_view = FindNode(key_path))
    {
        if (const auto* sub_table = node_view.as_table())
        {
            // toml::table을 복사하여 새로운 Config 객체 생성
            return Config(toml::table(*sub_table));
        }
    }
    return std::nullopt;
}

bool Config::WriteConfig(const VPath& config_file_path) const
{
    Path physical_path = config_file_path.ToPath();
    if (physical_path.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "Failed to resolve config file path: {}", config_file_path.ToString());
        return false;
    }

    const String physical_path_str = physical_path.ToString();
    std::ofstream file_stream(physical_path_str.CStr(), std::ios::binary | std::ios::trunc);
    if (!file_stream.is_open())
    {
        ConsoleLog(ELogLevel::Error, "Failed to open config file for writing: {}", physical_path_str);
        return false;
    }

    // config를 직렬화
    file_stream << config_table;

    if (file_stream.fail())
    {
        ConsoleLog(ELogLevel::Error, "Failed to write config file: {}", physical_path);
        file_stream.close();
        return false;
    }

    file_stream.close();
    if (file_stream.fail())
    {
        ConsoleLog(ELogLevel::Warning, "Potential issue closing file stream for: {}", physical_path);
    }

    return true;
}

toml::node_view<const toml::node> Config::FindNode(std::string_view path_str) const
{
    return config_table.at_path(path_str);
}
}  // namespace se::utility
