#include "SimpleEngine/Core/Config/ConfigFile.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Serialization/Serializer.h"
#include "SimpleEngine/Utility/Common.h"

#include <ostream>


namespace se
{
HashMap<String, toml::table> ConfigFile::table_cache;

Expected<ConfigFile, String> ConfigFile::Load(const VPath& config_file_path)
{
    const auto physical_path_opt = VFS::Resolve(config_file_path);
    if (!physical_path_opt.HasValue())
    {
        return Unexpected{ String::Format("Failed to resolve config file path: {}", config_file_path.ToString()) };
    }

    const String path_key = physical_path_opt->ToString();

    // 캐시에 있으면 캐시에서 반환 (복사)
    if (const auto table = table_cache.Find(path_key))
    {
        return ConfigFile{ toml::table{ *table }};
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
    table_cache.Insert(path_key, parsed);

    return ConfigFile{ std::move(parsed) };
}

bool ConfigFile::Save(const VPath& config_file_path) const
{
    const Path physical_path = VFS::ToPath(config_file_path);
    if (physical_path.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::Save: Failed to resolve config file path: {}", config_file_path);
        return false;
    }

    std::ostringstream oss;
    oss << root_table;

    const String& physical_path_str = physical_path.ToString();
    const Path temp_path = Path{ physical_path_str + ".tmp" };
    SE_SCOPE_DEFER_NAMED(rollback) {
        fs::Remove(temp_path);
    };

    if (!fs::WriteString(temp_path, oss.view()))
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::Save: Failed to write temp file: {}", temp_path.ToString());
        return false;
    }

    if (!fs::Rename(temp_path, physical_path))
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::Save: Failed to rename temp -> config: {} -> {}", temp_path.ToString(), physical_path.ToString());
        return false;
    }

    // 저장 성공 시 캐시 갱신
    table_cache.Insert(physical_path_str, root_table);

    rollback.Discard();
    return true;
}

void ConfigFile::InvalidateCache(const VPath& config_file_path)
{
    if (const auto resolved = VFS::Resolve(config_file_path))
    {
        table_cache.Remove(resolved->ToString());
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
    auto dot_pos = key_path.Find('.');

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

bool ConfigFile::ReadSection(StringView section_name, const SerializePlan& plan, void* out_value) const
{
    const toml::table* const section_table = FindSectionTable(section_name);
    if (!section_table)
    {
        return false;
    }

    TomlReader reader(*section_table);
    const auto result = serde::Deserialize(reader, plan, out_value);

    // 필드 이름을 바꿨거나 오타를 낸 키를 알아차리게 모두 남김
    for (const String& warning : reader.GetWarnings())
    {
        ConsoleLog(ELogLevel::Warning, "ConfigFile::GetSection: Section '{}': {}", section_name, warning);
    }

    if (result.HasError())
    {
        ConsoleLog(
            ELogLevel::Error,
            "ConfigFile::GetSection: Failed to read section '{}' at '{}', using default values. {}",
            section_name, result.Error().path, result.Error().message
        );
        return false;
    }
    return true;
}

void ConfigFile::WriteSection(StringView section_name, const SerializePlan& plan, const void* value)
{
    // 실패해도 기존 섹션이 바뀌지 않도록 새 테이블에 씀
    toml::table section_table;
    TomlWriter writer(section_table);
    if (const auto result = serde::Serialize(writer, plan, value); result.HasError())
    {
        ConsoleLog(
            ELogLevel::Error,
            "ConfigFile::SetSection: Failed to write section '{}' at '{}', the section is left unchanged. {}",
            section_name, result.Error().path, result.Error().message
        );
        return;
    }

    if (section_name.IsEmpty())
    {
        // 루트 테이블에 병합 (기존 값은 덮어씀)
        for (auto&& [key, val] : section_table)
        {
            root_table.insert_or_assign(key, std::move(val));
        }
    }
    else
    {
        root_table.insert_or_assign(section_name, std::move(section_table));
    }
}
} // namespace se
