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

    template <typename To>
    std::optional<To> GetValue(std::u8string_view key);

    template <typename To, typename U = To>
        requires std::is_convertible_v<U, To>
    To GetValueOr(std::u8string_view key, U&& default_val = U{});

private:
    explicit Config(toml::table&& table);

private:
    toml::table config_table;
};

template <typename To>
std::optional<To> Config::GetValue(std::u8string_view key)
{
    const std::string key_str = std::string(key.begin(), key.end());
    return config_table[key_str].value<To>();
}

template <typename To, typename U>
    requires std::is_convertible_v<U, To>
To Config::GetValueOr(std::u8string_view key, U&& default_val)
{
    if (auto val = GetValue<To>(key))
    {
        return *val;
    }
    return static_cast<To>(std::forward<U>(default_val));
}
}
