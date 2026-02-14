#pragma once
#include <type_traits>

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Reflection/TypeRegistry.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Traits/SerializationTraits.h"
#include "SimpleEngine/Utility/StringUtils.h"


namespace se
{
// forward declaration
class VPath;

/**
 * Reflection + Archive 기반 TOML 설정 파일 관리 클래스입니다.
 *
 * 리플렉션이 등록된 구조체를 통해 타입 안전하게 설정을 관리합니다.
 * 내부적으로 TomlReader/TomlWriter Archive를 사용하여 직렬화합니다.
 *
 * @code
 * // 1. 설정 구조체 정의
 * struct WindowSettings {
 *     uint32 width = 1280;
 *     uint32 height = 720;
 *     bool fullscreen = false;
 *     String title = "SimpleEngine";
 * };
 * SE_BEGIN_REFLECT(WindowSettings, meta::SerializeOnly)
 * SE_REFLECT_PROPERTY(width, meta::Property)
 * SE_REFLECT_PROPERTY(height, meta::Property)
 * SE_REFLECT_PROPERTY(fullscreen, meta::Property)
 * SE_REFLECT_PROPERTY(title, meta::Property)
 * SE_END_REFLECT(WindowSettings)
 *
 * // 2. 사용
 * auto config = ConfigFile::Load("Config://EngineConfig.toml").Value();
 * auto window = config.GetSection<WindowSettings>("window");
 * // window.width == 1280 (TOML에 있으면 그 값, 없으면 구조체 기본값)
 * config.SetSection(window, "window");  // 누락된 값 자동 생성
 * config.Save("Config://EngineConfig.toml");
 * @endcode
 */
class SE_CORE_API ConfigFile
{
public:
    ConfigFile() = default;
    ~ConfigFile() = default;

    ConfigFile(const ConfigFile&) = default;
    ConfigFile(ConfigFile&&) noexcept = default;
    ConfigFile& operator=(const ConfigFile&) = default;
    ConfigFile& operator=(ConfigFile&&) noexcept = default;

public:
    /**
     * TOML 파일을 로드하여 ConfigFile 객체를 생성합니다.
     * @param config_file_path TOML 파일의 가상 경로 (예: "Config://EngineConfig.toml")
     * @return 성공 시 ConfigFile 객체, 실패 시 에러 메시지 문자열
     */
    [[nodiscard]] static Expected<ConfigFile, String> Load(const VPath& config_file_path);

    /**
     * 현재 설정을 TOML 파일로 저장합니다.
     * @param config_file_path 저장할 파일의 가상 경로
     * @return 저장 성공 여부
     */
    [[nodiscard]] bool Save(const VPath& config_file_path) const;

public:
    /**
     * 지정된 섹션을 리플렉션 구조체로 역직렬화하여 반환합니다.
     * 섹션이 없거나 필드가 누락되면 구조체의 기본값이 유지됩니다.
     *
     * @tparam T 리플렉션이 등록된 구조체 타입 (SE_BEGIN_REFLECT 필수)
     * @param section_name TOML 테이블 이름. 비어있으면 루트 테이블에서 직접 역직렬화합니다.
     * @return 역직렬화된 구조체. 섹션이 없으면 기본 생성된 T를 반환합니다.
     */
    template <typename T>
    [[nodiscard]] T GetSection(StringView section_name = "") const;

    /**
     * 구조체를 직렬화하여 지정된 섹션에 저장합니다.
     *
     * @tparam T 리플렉션이 등록된 구조체 타입 (SE_BEGIN_REFLECT 필수)
     * @param settings 저장할 구조체
     * @param section_name TOML 테이블 이름. 비어있으면 루트 테이블에 직접 기록합니다.
     */
    template <typename T>
    void SetSection(const T& settings, StringView section_name = "");

    /**
     * 점(.) 구분 키 경로로 개별 스칼라 값을 가져옵니다.
     *
     * @tparam T 반환받을 값의 타입 (bool, 정수, 실수, se::String)
     * @param key 점(.)으로 구분된 키 경로 (예: "window.width")
     * @return 값이 존재하고 변환 가능하면 값을, 아니면 std::nullopt를 반환합니다.
     */
    template <typename T>
    [[nodiscard]] Optional<T> GetValue(StringView key) const;

    /**
     * 점(.) 구분 키 경로에 스칼라 값을 설정합니다.
     * 중간 경로의 테이블은 필요 시 자동 생성됩니다.
     *
     * @tparam T 설정할 값의 타입
     * @param key 점(.)으로 구분된 키 경로 (예: "window.width")
     * @param value 설정할 값
     * @return 설정 성공 여부
     */
    template <typename T>
    bool SetValue(StringView key, T&& value);

    /** 설정이 비어있는지 확인합니다. */
    [[nodiscard]] bool IsEmpty() const;

private:
    explicit ConfigFile(toml::table&& table);

    /**
     * section_name에 해당하는 TOML 테이블을 찾습니다.
     * @param section_name 테이블 이름. 비어있으면 루트 테이블을 반환합니다.
     * @return 테이블이 존재하면 포인터를, 아니면 nullptr를 반환합니다.
     */
    [[nodiscard]] const toml::table* FindSectionTable(StringView section_name) const;

    /**
     * 점(.) 구분 키 경로의 마지막 키에 대한 부모 테이블을 찾거나 생성합니다.
     * @param key_path 점(.)으로 구분된 키 경로
     * @param[out] out_final_key 마지막 키 이름
     * @return 부모 테이블 포인터. 경로가 잘못되면 nullptr.
     */
    [[nodiscard]] toml::table* NavigateOrCreate(StringView key_path, std::string_view& out_final_key);

private:
    toml::table root_table;
};


template <typename T>
T ConfigFile::GetSection(StringView section_name) const
{
    T result{};

    const toml::table* target = FindSectionTable(section_name);
    if (!target)
    {
        return result; // 섹션 미존재 → 기본 생성된 T 반환
    }

    TomlReader reader(*target);

    if constexpr (traits::Serializable<T>)
    {
        // ADL Serialize(Archive&, T&) 가 있는 타입
        reader << result;
    }
    else
    {
        // 리플렉션 등록된 타입 → TypeInfo::serialize 사용
        const TypeInfo& info = TypeRegistry::Get().FindChecked<T>();
        if (info.serialize)
        {
            info.serialize(reader, &result);
        }
        else
        {
            ConsoleLog(ELogLevel::Error, "ConfigFile::GetSection: Type '{}' has no serialization support.", info.name);
        }
    }

    return result;
}

template <typename T>
void ConfigFile::SetSection(const T& settings, StringView section_name)
{
    toml::table section_table;
    TomlWriter writer(section_table);

    if constexpr (traits::Serializable<T>)
    {
        writer << const_cast<T&>(settings);
    }
    else
    {
        const TypeInfo& info = TypeRegistry::Get().FindChecked<T>();
        if (info.serialize)
        {
            info.serialize(writer, const_cast<void*>(static_cast<const void*>(&settings)));
        }
        else
        {
            ConsoleLog(ELogLevel::Error, "ConfigFile::SetSection: Type '{}' has no serialization support.", info.name);
            return;
        }
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
        root_table.insert_or_assign(std::string_view{ section_name }, std::move(section_table));
    }
}

template <typename T>
Optional<T> ConfigFile::GetValue(StringView key) const
{
    const auto node = root_table.at_path(std::string_view{ key });
    if (!node)
    {
        return std::nullopt;
    }

    if constexpr (std::same_as<T, String>)
    {
        if (const auto val = node.value<std::u8string>())
        {
            return StringUtils::ToString(*val);
        }
        return std::nullopt;
    }
    else
    {
        return node.value<T>();
    }
}

template <typename T>
bool ConfigFile::SetValue(StringView key, T&& value)
{
    if (key.IsEmpty())
    {
        ConsoleLog(ELogLevel::Error, "ConfigFile::SetValue: Key path cannot be empty.");
        return false;
    }

    std::string_view out_final_key;
    toml::table* parent = NavigateOrCreate(key, out_final_key);
    if (!parent || out_final_key.empty())
    {
        return false;
    }

    parent->insert_or_assign(out_final_key, std::forward<T>(value));
    return true;
}
}  // namespace se
