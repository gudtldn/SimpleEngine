#pragma once

#include "SimpleEngine/Core/Container/ArrayView.h"
#include "SimpleEngine/Core/Container/HashMap.h"
#include "SimpleEngine/Core/Container/Optional.h"
#include "SimpleEngine/Core/Types/VPath.h"
#include "SimpleEngine/Core/Types/Path.h"

#include "tracy/Tracy.hpp"

#include <shared_mutex>


namespace se
{
/**
 * 가상 파일 시스템 (Virtual File System)
 *
 * VPath(가상 경로)를 실제 물리적 파일 시스템 경로로 변환합니다.
 * 스킴(scheme)을 물리적 경로에 마운트하여 가상 경로 시스템을 구축합니다.
 *
 * @code
 * // 마운트
 * VFS::Get().Mount("Assets", "C:/Project/Assets");
 *
 * // 해석
 * if (auto physical = VPath("Assets://Textures/Player.png").Resolve())
 * {
 *     // physical 사용
 * }
 * @endcode
 */
class SE_CORE_API VFS
{
    VFS() = default;

public:
    ~VFS() = default;

    // 복사 및 이동 불가 (싱글톤)
    VFS(const VFS&) = delete;
    VFS& operator=(const VFS&) = delete;
    VFS(VFS&&) = delete;
    VFS& operator=(VFS&&) = delete;

public:
    /** 싱글톤 인스턴스를 반환합니다. */
    [[nodiscard]] static VFS& Get();

    /**
     * 가상 경로 스킴(scheme)을 물리적 경로에 마운트합니다.
     *
     * @param scheme 마운트할 스킴 (예: "Assets", "Config")
     * @param physical_path 매핑할 실제 디스크 경로
     * @param priority 우선순위. 높을수록 Unresolve 시 먼저 고려됩니다. (모딩 지원용)
     */
    void Mount(StringView scheme, const Path& physical_path, int32 priority = 0);

    /**
     * 마운트된 스킴을 해제합니다.
     * @param scheme 해제할 스킴
     */
    void Unmount(StringView scheme);

    /**
     * VPath를 물리적 경로로 해석합니다.
     *
     * @param virtual_path 해석할 가상 경로
     * @param check_existence true이면 파일이 존재할 때만 반환, false이면 최우선 경로 반환
     * @return 물리적 경로. 해석 실패 시 NullOpt
     */
    [[nodiscard]] Optional<Path> Resolve(const VPath& virtual_path, bool check_existence = true) const;

    /**
     * 물리적 경로를 가상 경로로 역해석합니다.
     *
     * @param physical_path 역해석할 물리적 경로
     * @return 가상 경로. 해석 실패 시 NullOpt
     */
    [[nodiscard]] Optional<VPath> Unresolve(const Path& physical_path) const;

public:
    /**
     * 지정된 스킴들의 마운트 포인트 물리 디렉토리가 존재하지 않으면 생성합니다.
     * Cache, Logs 등 쓰기 대상 스킴에 사용합니다.
     * @param schemes 디렉토리를 보장할 스킴 목록
     */
    void EnsureDirectories(ArrayView<const StringView> schemes);

    /**
     * 등록된 모든 마운트 포인트를 순회합니다.
     * @param visitor 콜백 함수 (scheme, physical_path, priority)
     */
    template <typename Fn>
        requires std::invocable<Fn, StringView, const Path&, int32>
    void VisitMounts(Fn&& visitor) const;

private:
    struct MountPoint
    {
        Path physical_path;
        int32 priority = 0;

        auto operator<=>(const MountPoint& other) const { return priority <=> other.priority; }
    };

    HashMap<StringName, Array<MountPoint>> mount_points;
    mutable TracySharedLockable(std::shared_mutex, mutex);
};


// === Template Implementation ===

template <typename Fn>
    requires std::invocable<Fn, StringView, const Path&, int32>
void VFS::VisitMounts(Fn&& visitor) const
{
    std::shared_lock lock(mutex);
    for (const auto& [scheme, points] : mount_points)
    {
        for (const auto& [physical_path, priority] : points)
        {
            std::invoke(std::forward<Fn>(visitor), StringView{ scheme.CStr() }, physical_path, priority);
        }
    }
}
}  // namespace se
