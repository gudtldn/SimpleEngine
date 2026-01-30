#include "SimpleEngine/Utility/PathResolver.h"

#include <algorithm>
#include <format>
#include <functional>
#include <shared_mutex>

#include "Core/Logging/Logging.h"
#include "Utility/FileSystem.h"
#include "Utility/StringUtils.h"


namespace se::utility
{
PathResolver& PathResolver::Get()
{
    static PathResolver instance;
    return instance;
}

void PathResolver::Mount(const StringName& scheme, const Path& physical_path, int32 priority)
{
    std::unique_lock lock(mutex);

    // 경로를 정규화하여 저장
    Path abs_path = physical_path.GetNormalized();

    Array<MountPoint>& points = mount_points[scheme];
    const MountPoint* it = std::ranges::find_if(points, [&](const MountPoint& point)
    {
        return point.priority == priority && point.physical_path == abs_path;
    });

    // 이미 존재하는 경우
    if (it != points.end())
    {
        return;
    }

    points.Push({
        .physical_path = std::move(abs_path),
        .priority = priority
    });

    // 우선순위가 높은 것이 앞에 오도록 정렬 (stable_sort로 순서 유지)
    std::ranges::stable_sort(points, std::greater{});
    ConsoleLog(ELogLevel::Info, "Virtual path scheme '{}://' mounted at '{}', priority: {}", scheme, physical_path, priority);
}

void PathResolver::Unmount(const StringName& scheme)
{
    std::unique_lock lock(mutex);
    mount_points.Remove(scheme);
}

Optional<Path> PathResolver::Resolve(const VPath& virtual_path, bool check_existence) const
{
    if (!virtual_path.IsValid() || !virtual_path.HasScheme())
    {
        return std::nullopt; // 스키마가 없으면 해석 불가
    }

    std::shared_lock lock(mutex);

    const std::string_view scheme = virtual_path.GetScheme();
    const Optional point_opt = mount_points.Find(scheme);

    if (!point_opt.HasValue() || point_opt->IsEmpty())
    {
        ConsoleLog(ELogLevel::Warning, "Scheme '{}' is not mounted.", scheme);
        return std::nullopt;
    }

    // 경로 부분에서 맨 앞의 '/' 제거
    std::string_view relative_part = virtual_path.GetPathPart();
    if (relative_part.starts_with('/'))
    {
        relative_part.remove_prefix(1);
    }

    // 모든 마운트 포인트를 순회하며 파일이 실제로 존재하는지 확인 (Mod Fallback)
    for (const MountPoint& mount_point : *point_opt)
    {
        Path candidate = mount_point.physical_path / relative_part;

        if (check_existence)
        {
            if (candidate.Exists())
            {
                return candidate;
            }
        }
        else
        {
            // 가장 우선순위가 높은(첫 번째) 마운트 포인트를 사용하여 경로를 조합
            return candidate;
        }
    }

    ConsoleLog(ELogLevel::Warning, "File '{}' not found in any mounted path for scheme '{}'.", virtual_path.ToString(), scheme);
    return std::nullopt;
}

Optional<VPath> PathResolver::Unresolve(const Path& physical_path) const
{
    std::shared_lock lock(mutex);

    Path abs_input = FileSystem::Absolute(physical_path);
    abs_input.Normalize();
    const String input_str = abs_input.ToString();
    const std::string_view input_view{ input_str };

    const StringName* best_scheme = nullptr;
    const MountPoint* best_mount_point = nullptr;
    usize best_match_len = 0;

    for (const auto& [scheme, points] : mount_points)
    {
        for (const MountPoint& point : points)
        {
            const String root_str = point.physical_path.ToString();
            const std::string_view root_view{ root_str };

            // 물리적 경로가 마운트 포인트의 하위 경로인지 확인
            if (input_view.starts_with(root_view)) // 접두사가 일치하는 경우
            {
                if (
                    input_view.size() == root_view.size()
                    || input_view[root_view.size()] == '/'
                ) {
                    // 더 긴 경로가 매칭되거나 (하위 폴더 마운트 우선), 길이는 같은데 우선순위가 높은 경우 선택
                    if (
                        root_view.size() > best_match_len
                        || (root_view.size() == best_match_len && (!best_mount_point || point.priority > best_mount_point->priority))
                    ) {
                        best_match_len = root_view.size();
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

    return std::nullopt;
}
}  // namespace se::utility
