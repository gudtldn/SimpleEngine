#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include <ostream>

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Utility/Common.h"


namespace se
{
HashMap<String, toml::table> ConfigFile::table_cache;

Expected<ConfigFile, String> ConfigFile::Load(const VPath& config_file_path)
{
    const Optional physical_path_opt = config_file_path.Resolve();
    if (!physical_path_opt.HasValue())
    {
        return Unexpected{ String::Format("Failed to resolve config file path: {}", config_file_path.ToString()) };
    }

    const String path_key = physical_path_opt->ToString();

    // 캐시에 있으면 캐시에서 반환 (복사)
    if (const Optional table_opt = table_cache.Find(path_key))
    {
        return ConfigFile{ toml::table{ *table_opt }};
    }

    toml::parse_result result = toml::parse_file(path_key.CStr());
    if (result.failed())
    {
        return Unexpected{
            String::Format("Failed to parse TOML file '{}': {}", config_file_path.ToString(), result.error().description())
        };
    }

    toml::table parsed = std::move(result).table();

    // 캐시에 저장 (복사본)
    table_cache[path_key] = parsed;

    return ConfigFile{ std::move(parsed) };
}

bool ConfigFile::Save(const VPath& config_file_path) const
{
    const Path physical_path = config_file_path.ToPath();
    if (physical_path.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::Save: Failed to resolve config file path: {}", config_file_path);
        return false;
    }

    std::ostringstream oss;
    oss << root_table;

    const String physical_path_str = physical_path.ToString();
    const Path temp_path = Path{ physical_path_str + ".tmp" };
    SE_SCOPE_DEFER_NAMED(rollback) {
        FileSystem::Remove(temp_path);
    };

    if (!FileSystem::WriteString(temp_path, oss.view()))
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::Save: Failed to write temp file: {}", temp_path.ToString());
        return false;
    }

    if (!FileSystem::Rename(temp_path, physical_path))
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::Save: Failed to rename temp -> config: {} -> {}", temp_path.ToString(), physical_path.ToString());
        return false;
    }

    // 저장 성공 시 캐시 갱신
    table_cache[physical_path_str] = root_table;

    rollback.Discard();
    return true;
}

void ConfigFile::InvalidateCache(const VPath& config_file_path)
{
    if (const Optional resolved_opt = config_file_path.Resolve())
    {
        table_cache.Remove(resolved_opt->ToString());
    }
}

void ConfigFile::InvalidateAllCaches()
{
    table_cache.Clear();
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

    const toml::node* node = root_table.get(section_name);
    if (node && node->is_table())
    {
        return node->as_table();
    }
    return nullptr;
}

toml::table* ConfigFile::NavigateOrCreate(StringView key_path, StringView& out_final_key)
{
    toml::table* current = &root_table;

    usize current_pos = 0;
    Optional dot_pos = key_path.Find('.');

    while (dot_pos.HasValue())
    {
        const StringView segment = key_path.Substr(current_pos, *dot_pos - current_pos);

        if (toml::node* node = current->get(segment))
        {
            if (node->is_table())
            {
                current = node->as_table();
            }
            else
            {
                ConsoleLog(
                    ELogLevel::Error,
                    "ConfigFile::SetValue: Path conflict at '{}' in '{}'. Expected a table.", segment, key_path
                );
                return nullptr;
            }
        }
        else
        {
            auto [it, success] = current->emplace(segment, toml::table{});
            if (!success || !it->second.is_table())
            {
                ConsoleLog(
                    ELogLevel::Error,
                    "ConfigFile::SetValue: Failed to create intermediate table at '{}' in '{}'.", segment, key_path
                );
                return nullptr;
            }
            current = it->second.as_table();
        }

        current_pos = *dot_pos + 1;
        dot_pos = key_path.Find('.', current_pos);
    }

    out_final_key = key_path.Substr(current_pos);
    if (out_final_key.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::SetValue: Key path '{}' ends with delimiter.", key_path);
        return nullptr;
    }

    return current;
}
}  // namespace se
