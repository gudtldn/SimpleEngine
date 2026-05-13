#include "SimpleEngine/Core/FileSystem/VFS.h"
#include "SimpleEngine/Core/Logging/Logging.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"

#include <algorithm>
#include <functional>
#include <shared_mutex>


namespace se
{
VFS& VFS::Get()
{
    static VFS instance;
    return instance;
}

Optional<Path> VFS::Resolve(const VPath& vpath)
{
    return Get().ResolveImpl(vpath, true);
}

Path VFS::ToPath(const VPath& vpath)
{
    return Get().ResolveImpl(vpath, false).ValueOrDefault();
}

Optional<VPath> VFS::Unresolve(const Path& path)
{
    return Get().UnresolveImpl(path);
}

bool VFS::Exists(const VPath& vpath)
{
    return Get().ResolveImpl(vpath, true).HasValue();
}

void VFS::Mount(StringView scheme, const Path& physical_path, i32 priority)
{
    std::unique_lock lock(mutex);

    const StringName scheme_name{ scheme };
    Array<MountPoint>& points = mount_points[scheme_name];

    // 이미 존재하는지 확인
    const MountPoint* it = std::ranges::find_if(points, [&](const MountPoint& point)
    {
        return point.priority == priority && point.physical_path == physical_path;
    });

    if (it != points.end())
    {
        return;
    }

    points.Push({
        .physical_path = FileSystem::Absolute(physical_path),
        .priority = priority
    });

    // 우선순위가 높은 것이 앞에 오도록 정렬
    std::ranges::stable_sort(points, std::greater{});
    ConsoleLog(ELogLevel::Info, "VFS: Mounted '{}://' -> '{}' (priority: {})", scheme, physical_path, priority);
}

void VFS::Unmount(StringView scheme)
{
    std::unique_lock lock(mutex);
    mount_points.Remove(StringName{ scheme });
    ConsoleLog(ELogLevel::Info, "VFS: Unmounted '{}://'", scheme);
}

void VFS::EnsureDirectories(ArrayView<const StringView> schemes)
{
    std::shared_lock lock(mutex);

    for (const StringView scheme : schemes)
    {
        const auto point_opt = mount_points.Find(scheme);
        if (!point_opt.HasValue())
        {
            continue;
        }

        for (const MountPoint& point : *point_opt)
        {
            if (!point.physical_path.Exists())
            {
                FileSystem::CreateDirectories(point.physical_path);
                ConsoleLog(ELogLevel::Info, "VFS: Created directory for '{}://': '{}'", scheme, point.physical_path);
            }
        }
    }
}

Optional<Path> VFS::ResolveImpl(const VPath& virtual_path, bool check_existence) const
{
    if (!virtual_path.IsValid() || !virtual_path.HasScheme())
    {
        return NullOpt;
    }

    std::shared_lock lock(mutex);

    const StringView scheme = virtual_path.GetScheme();
    const auto point_opt = mount_points.Find(scheme);

    if (!point_opt.HasValue() || point_opt->IsEmpty())
    {
        ConsoleLog(ELogLevel::Warning, "VFS: Scheme '{}' is not mounted.", scheme);
        return NullOpt;
    }

    // 경로 부분에서 맨 앞의 '/' 제거
    StringView relative_part = virtual_path.GetPathPart();
    if (relative_part.StartsWith('/'))
    {
        relative_part = relative_part.Substr(1);
    }

    // 모든 마운트 포인트를 순회하며 파일이 실제로 존재하는지 확인 (Mod Fallback)
    for (const MountPoint& mount_point : *point_opt)
    {
        Path candidate = mount_point.physical_path / relative_part;

        // ".."을 포함하는 가상 경로가 마운트 포인트 외부로 탈출하는 것을 방지
        if (!candidate.IsSubPathOf(mount_point.physical_path))
        {
            ConsoleLog(
                ELogLevel::Warning,
                "VFS: Path traversal blocked. '{}' escapes mount point '{}'.",
                virtual_path.ToString(), mount_point.physical_path
            );
            continue;
        }

        if (check_existence)
        {
            if (candidate.Exists())
            {
                return candidate;
            }
        }
        else
        {
            // 가장 우선순위가 높은(첫 번째) 마운트 포인트를 사용
            return candidate;
        }
    }

    ConsoleLog(ELogLevel::Warning, "VFS: '{}' not found in any mounted path for scheme '{}'.", virtual_path.ToString(), scheme);
    return NullOpt;
}

// TODO: [Performance] 모든 마운트 포인트를 선형 탐색함. 마운트 수가 많아지면
//       경로 접두사 기반 trie 또는 정렬된 배열 + 이진 탐색으로 전환 검토.
Optional<VPath> VFS::UnresolveImpl(const Path& physical_path) const
{
    std::shared_lock lock(mutex);

    Path abs_input = FileSystem::Absolute(physical_path);
    const String& input_str = abs_input.ToString();
    const StringView input_view{ input_str };

    const StringName* best_scheme = nullptr;
    const MountPoint* best_mount_point = nullptr;
    usize best_match_len = 0;

    for (const auto& [scheme, points] : mount_points)
    {
        for (const MountPoint& point : points)
        {
            const String& root_str = point.physical_path.ToString();
            const StringView root_view{ root_str };

            // 물리적 경로가 마운트 포인트의 하위 경로인지 확인
            if (input_view.StartsWith(root_view))
            {
                if (
                    input_view.ByteLen() == root_view.ByteLen()
                    || input_view[root_view.ByteLen()] == '/'
                ) {
                    // 더 긴 경로가 매칭되거나, 같은 길이면 우선순위가 높은 것 선택
                    if (
                        root_view.ByteLen() > best_match_len
                        || (root_view.ByteLen() == best_match_len && (!best_mount_point || point.priority > best_mount_point->priority))
                    ) {
                        best_match_len = root_view.ByteLen();
                        best_scheme = &scheme;
                        best_mount_point = &point;
                    }
                }
            }
        }
    }

    if (best_scheme && best_mount_point)
    {
        // 상대 경로 추출
        const Path relative_path = abs_input.RelativeTo(best_mount_point->physical_path).Value();
        const bool is_root = relative_path == ".";

        String relative_str = is_root ? String{} : relative_path.ToString();
        return VPath{ String::Format("{}://{}", best_scheme->ToString(), relative_str) };
    }

    return NullOpt;
}
} // namespace se
