#include "Utility/Config.h"

#include "Utility/FileUtils.h"
#include "Utility/PathResolver.h"


namespace
{
se::utility::PathResolver& resolver = se::utility::PathResolver::Get();
}

namespace se::utility
{
ParseResult Config::ReadConfig(const VPath& config_file_path)
{
    Optional physical_path_opt = resolver.Resolve(config_file_path);
    if (!physical_path_opt.HasValue())
    {
        // TODO: 에러 반환타입 리펙토링 toml::parse_result가 아닌, 새로운 타입으로
        return std::unexpected{ toml::parse_file("").error() };
    }

    toml::parse_result result = toml::parse_file(physical_path_opt->generic_string());
    if (result.failed())
    {
        return std::unexpected{ std::move(result).error() };
    }
    return Config{ std::move(result).table() };
}

Optional<Config> Config::GetTable(std::string_view key_path) const
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

bool Config::WriteConfig(const VPath& config_file_path) const
{
    Optional physical_path_opt = resolver.Resolve(config_file_path, false);
    if (!physical_path_opt.HasValue())
    {
        ConsoleLog(ELogLevel::Error, "Failed to resolve config file path: {}", config_file_path.ToString());
        return false;
    }

    const std::string physical_path = physical_path_opt->generic_string();
    std::ofstream file_stream(physical_path, std::ios::binary | std::ios::trunc);
    if (!file_stream.is_open())
    {
        ConsoleLog(ELogLevel::Error, "Failed to open config file for writing: {}", physical_path);
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
}
