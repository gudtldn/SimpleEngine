module;
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
module SimpleEngine.Config;

import SimpleEngine.Logging;


namespace se::config
{
ParseResult Config::ReadConfig(const std::filesystem::path& config_file_path)
{
    toml::parse_result result = toml::parse_file(config_file_path.generic_string());
    if (result.failed())
    {
        const toml::parse_error& err = result.error();
        ConsoleLog(ELogLevel::Error, u8"Failed to parse config file: {}", err.description());

        return std::unexpected{err};
    }

    config_table = std::move(result.table());
    return {};
}
}
