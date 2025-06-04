module;
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
export module SimpleEngine.Config;

import SimpleEngine.Platform.Types;
import std;


namespace se::config
{
export class Config;
export using ParseResult = std::expected<Config, toml::parse_error>;

class Config
{
public:
    static ParseResult ReadConfig(const std::filesystem::path& config_file_path);

private:
    toml::table config_table;
};
}
