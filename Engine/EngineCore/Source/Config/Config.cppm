module;
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
export module SimpleEngine.Config;

import SimpleEngine.Platform.Types;
import std;


namespace se::config
{
using TomlResult = std::expected<void, toml::parse_error>;

export class Config
{
    TomlResult ReadConfig(const std::filesystem::path& config_file_path);

private:
    toml::table config_table;
};
}
