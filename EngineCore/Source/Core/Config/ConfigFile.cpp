#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/Types/VPath.h"


namespace se
{
Expected<ConfigFile, String> ConfigFile::Load(const VPath& config_file_path)
{
    const Optional physical_path_opt = config_file_path.Resolve();
    if (!physical_path_opt.HasValue())
    {
        return Unexpected{ String::Format("Failed to resolve config file path: {}", config_file_path.ToString()) };
    }

    toml::parse_result result = toml::parse_file(physical_path_opt->ToString().CStr());
    if (result.failed())
    {
        return Unexpected{ String::Format("Failed to parse TOML file '{}': {}",
            config_file_path.ToString(), result.error().description()) };
    }

    return ConfigFile{ std::move(result).table() };
}

bool ConfigFile::Save(const VPath& config_file_path) const
{
    const Path physical_path = config_file_path.ToPath();
    if (physical_path.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::Save: Failed to resolve config file path: {}",
            config_file_path.ToString());
        return false;
    }

    const String physical_path_str = physical_path.ToString();
    std::ofstream file_stream(physical_path_str.CStr(), std::ios::binary | std::ios::trunc);
    if (!file_stream.is_open())
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::Save: Failed to open file for writing: {}", physical_path_str);
        return false;
    }

    file_stream << root_table;

    if (file_stream.fail())
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::Save: Failed to write file: {}", physical_path_str);
        return false;
    }

    file_stream.close();
    return !file_stream.fail();
}

bool ConfigFile::IsEmpty() const
{
    return root_table.empty();
}

ConfigFile::ConfigFile(toml::table&& table)
    : root_table(std::move(table))
{
}

const toml::table* ConfigFile::FindSectionTable(StringView section_name) const
{
    if (section_name.IsEmpty())
    {
        return &root_table;
    }

    const toml::node* node = root_table.get(std::string_view{ section_name });
    if (node && node->is_table())
    {
        return node->as_table();
    }
    return nullptr;
}

toml::table* ConfigFile::NavigateOrCreate(StringView key_path, std::string_view& out_final_key)
{
    const std::string_view full_path = key_path;
    toml::table* current = &root_table;
    usize current_pos = 0;
    usize dot_pos = full_path.find('.');

    while (dot_pos != std::string_view::npos)
    {
        const std::string_view segment{ full_path.substr(current_pos, dot_pos - current_pos) };

        if (toml::node* node = current->get(segment))
        {
            if (node->is_table())
            {
                current = node->as_table();
            }
            else
            {
                ConsoleLog(ELogLevel::Error,
                    "ConfigFile::SetValue: Path conflict at '{}' in '{}'. Expected a table.",
                    segment, key_path);
                return nullptr;
            }
        }
        else
        {
            auto [it, success] = current->emplace(segment, toml::table{});
            if (!success || !it->second.is_table())
            {
                ConsoleLog(ELogLevel::Error,
                    "ConfigFile::SetValue: Failed to create intermediate table at '{}' in '{}'.",
                    segment, key_path);
                return nullptr;
            }
            current = it->second.as_table();
        }

        current_pos = dot_pos + 1;
        dot_pos = full_path.find('.', current_pos);
    }

    out_final_key = full_path.substr(current_pos);
    if (out_final_key.empty())
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::SetValue: Key path '{}' ends with delimiter.", key_path);
        return nullptr;
    }

    return current;
}
}  // namespace se
