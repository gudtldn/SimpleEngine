#pragma once

#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Container/String.h"
#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Error/Expected.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/Serialization/SerializePlan.h"
#include "SimpleEngine/Core/Serialization/TomlArchive.h"
#include "SimpleEngine/Utility/StringUtils.h"

#include <concepts>


namespace se
{
// forward declaration
class VPath;

/**
 * Reflection + serde 기반 TOML 설정 파일 관리 클래스입니다.
 *
 * 리플렉션이 등록된 구조체를 통해 타입 안전하게 설정을 관리합니다.
 * 내부적으로 TomlReader/TomlWriter와 serde::Deserialize/Serialize를 사용하여 직렬화합니다.
 *
 * @code
 * // 1. 설정 구조체 정의
 * struct WindowSettings {
 *     u32 width = 1280;
 *     u32 height = 720;
 *     bool fullscreen = false;
 *     String title = "SimpleEngine";
 * };
 * SE_DECLARE_REFLECTION(WindowSettings) // 헤더
 *
 * SE_REFLECT_BEGIN(WindowSettings)      // .cpp
 *     SE_FIELD(width)
 *     SE_FIELD(height)
 *     SE_FIELD(fullscreen)
 *     SE_FIELD(title)
 * SE_REFLECT_END()
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
    ConfigFile& operator=(const ConfigFile&) = default;
    ConfigFile(ConfigFile&&) noexcept = default;
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

    /**
     * 특정 파일의 캐시를 무효화합니다.
     * 다음 Load 호출 시 디스크에서 다시 읽습니다.
     */
    static void InvalidateCache(const VPath& config_file_path);

    /** 모든 캐시를 무효화합니다. */
    static void InvalidateAllCaches();

public:
    /**
     * 지정된 섹션을 리플렉션 구조체로 역직렬화하여 반환합니다.
     * 섹션이 없거나 필드가 누락되면 구조체의 기본값이 유지됩니다.
     * T에 없는 키는 경고로, 읽기 실패(값의 종류가 다르거나 범위를 벗어남)는 오류로 로그에 남깁니다.
     *
     * @tparam T 리플렉션이 등록된 구조체 타입 (SE_REFLECT_BEGIN 필수)
     * @param section_name TOML 테이블 이름. 비어있으면 루트 테이블에서 직접 역직렬화합니다.
     * @return 역직렬화된 구조체. 섹션이 없거나 읽기에 실패하면 기본 생성된 T를 반환합니다.
     */
    template <typename T>
    [[nodiscard]] T GetSection(StringView section_name = "") const;

    /**
     * 구조체를 직렬화하여 지정된 섹션에 저장합니다.
     * 쓰기에 실패하면 오류를 로그로 남기고 기존 내용을 바꾸지 않습니다.
     *
     * @tparam T 리플렉션이 등록된 구조체 타입 (SE_REFLECT_BEGIN 필수)
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
     * @return 값이 존재하고 변환 가능하면 값을, 아니면 NullOpt를 반환합니다.
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

    /**
     * 지정된 섹션의 모든 문자열 key-value 쌍을 순회합니다.
     * VFS 마운트 등 동적으로 key를 읽어야 할 때 사용합니다.
     *
     * @tparam Fn (StringView key, StringView value) 시그니처의 호출 가능 타입
     * @param section_name TOML 테이블 이름
     * @param visitor 각 문자열 항목에 대해 호출될 콜백
     */
    template <typename Fn>
        requires std::invocable<Fn, StringView, StringView>
    void VisitSectionEntries(StringView section_name, Fn&& visitor) const;

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
    [[nodiscard]] toml::table* NavigateOrCreate(StringView key_path, StringView& out_final_key);

    /**
     * section_name 섹션을 plan으로 읽어 out_value에 채웁니다.
     * 섹션에 있는데 타입에 없는 키는 경고로, 읽기 실패는 오류로 로그에 남깁니다.
     * @return 섹션이 있고 읽기에 성공하면 true. 읽기에 실패하면 out_value의 내용은 보장하지 않습니다.
     */
    [[nodiscard]] bool ReadSection(StringView section_name, const SerializePlan& plan, void* out_value) const;

    /**
     * value를 plan으로 새 테이블에 쓴 뒤 section_name 섹션을 그 테이블로 교체합니다.
     * section_name이 비어있으면 루트 테이블에 키 단위로 덮어씁니다. 쓰기에 실패하면 오류를 로그로 남기고 아무것도 바꾸지 않습니다.
     */
    void WriteSection(StringView section_name, const SerializePlan& plan, const void* value);

private:
    /** 파일 경로 -> 파싱된 TOML 테이블 캐시 (물리 경로 기준) */
    static HashMap<String, toml::table> table_cache;

    toml::table root_table;
};


template <typename T>
T ConfigFile::GetSection(StringView section_name) const
{
    // 읽다가 실패하면 내용을 보장하지 않으므로 새 객체에 읽고, 성공했을 때만 돌려줌
    T loaded{};
    if (!ReadSection(section_name, SerializePlan::Of<T>(), &loaded))
    {
        return T{}; // 섹션 미존재 또는 읽기 실패 -> 기본 생성된 T 반환
    }
    return loaded;
}

template <typename T>
void ConfigFile::SetSection(const T& settings, StringView section_name)
{
    WriteSection(section_name, SerializePlan::Of<T>(), &settings);
}

template <typename T>
Optional<T> ConfigFile::GetValue(StringView key) const
{
    const auto node = root_table.at_path(key);
    if (!node)
    {
        return NullOpt;
    }

    if constexpr (std::same_as<T, String>)
    {
        if (const auto val_opt = node.value<std::u8string>())
        {
            return str::ToString(*val_opt);
        }
        return NullOpt;
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

    StringView out_final_key;
    toml::table* parent = NavigateOrCreate(key, out_final_key);
    if (!parent || out_final_key.IsEmpty())
    {
        return false;
    }

    if constexpr (std::same_as<T, String>)
    {
        std::u8string str{ reinterpret_cast<char8_t*>(value.Data()), value.ByteLen() };
        parent->insert_or_assign(out_final_key, str);
    }
    else
    {
        parent->insert_or_assign(out_final_key, std::forward<T>(value));
    }
    return true;
}

template <typename Fn>
    requires std::invocable<Fn, StringView, StringView>
void ConfigFile::VisitSectionEntries(StringView section_name, Fn&& visitor) const
{
    const toml::table* target = FindSectionTable(section_name);
    if (!target)
    {
        return;
    }

    for (const auto& [key, value] : *target)
    {
        if (const auto* str = value.as_string())
        {
            const StringView key_sv{ key };
            const StringView val_sv{ str->get() };
            visitor(key_sv, val_sv);
        }
    }
}
} // namespace se
