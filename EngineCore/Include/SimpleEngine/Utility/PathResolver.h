#pragma once

#include <filesystem>
#include <shared_mutex>
#include <unordered_map>

#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/VPath.h"

#include "tracy/Tracy.hpp"


namespace se::utility
{
/**
 * VPath(가상 경로)를 실제 물리적 파일 시스템 경로로 변환하는 유틸리티 클래스.
 * 엔진 초기화 시점에 설정되어 전역적으로 경로 해석을 제공합니다.
 */
class SE_CORE_API PathResolver
{
    PathResolver() = default;

public:
    ~PathResolver() = default;

    // 복사 및 이동 불가
    PathResolver(const PathResolver&) = delete;
    PathResolver& operator=(const PathResolver&) = delete;
    PathResolver(PathResolver&&) = delete;
    PathResolver& operator=(PathResolver&&) = delete;

public:
    [[nodiscard]] static PathResolver& Get();

    /**
     * 가상 경로 스키마(scheme)을 실제 물리적 경로에 마운트합니다.
     * @param scheme 마운트할 스키마 (예: "Assets", "Config")
     * @param physical_path 매핑할 실제 디스크 경로
     * @param priority 우선순위. 숫자가 높을수록 Unresolve 시 먼저 고려됩니다. (모딩 지원용)
     */
    void Mount(const StringName& scheme, const std::filesystem::path& physical_path, int32 priority = 0);

    /**
     * 마운트된 스키마를 해제합니다.
     * @param scheme 해제할 스키마
     */
    void Unmount(const StringName& scheme);

    /**
     * VPath를 실제 물리적 경로로 해석합니다.
     * @param virtual_path 해석할 가상 경로
     * @param check_existence (기본값: true) true이면 파일이 실제로 존재할 때만 경로를 반환합니다. false이면 존재 여부와 상관없이 최우선 경로를 반환합니다.
     * @return 해당하는 물리적 경로. 유효하지 않으면 std::nullopt를 반환합니다.
     */
    [[nodiscard]] Optional<std::filesystem::path> Resolve(const VPath& virtual_path, bool check_existence = true) const;

    /**
     * 물리적 경로를 가장 적합한 VPath로 역해석합니다.
     * @param physical_path 역해석할 물리적 경로
     * @return 해당하는 가상 경로. 유효하지 않으면 std::nullopt를 반환합니다.
     */
    [[nodiscard]] Optional<VPath> Unresolve(const std::filesystem::path& physical_path) const;

private:
    struct MountPoint
    {
        std::filesystem::path physical_path;
        int32 priority = 0;

        auto operator<=>(const MountPoint& other) const { return priority <=> other.priority; }
    };

    unordered_map<StringName, std::vector<MountPoint>> mount_points;
    mutable TracySharedLockable(std::shared_mutex, mutex);
};
}
