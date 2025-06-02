module;
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
export module SimpleEngine.Config;

import SimpleEngine.Platform.Types;
import std;


namespace se::config
{
export using ParseResult = std::expected<void, toml::parse_error>;

export class Config
{
public:
    ParseResult ReadConfig(const std::filesystem::path& config_file_path);

private:
    toml::table config_table;
};
}
