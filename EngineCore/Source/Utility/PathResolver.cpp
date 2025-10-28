#include "SimpleEngine/Utility/PathResolver.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <functional>
#include <shared_mutex>

#include "Core/Logging/Logging.h"
#include "Utility/StringUtils.h"


namespace se::utility
{
PathResolver& PathResolver::Get()
{
    static PathResolver instance;
    return instance;
}

void PathResolver::Mount(const StringName& scheme, const std::filesystem::path& physical_path, int priority)
{
    std::unique_lock lock(mutex);

    // 경로를 정규화하여 저장
    auto normalized_path = std::filesystem::absolute(physical_path);

    Array<MountPoint>& points = mount_points[scheme];
    points.Push({
        .physical_path = std::move(normalized_path),
        .priority = priority
    });

    // 우선순위가 높은 것이 앞에 오도록 정렬 (stable_sort로 순서 유지)
    std::ranges::stable_sort(points, std::greater{});
}

void PathResolver::Unmount(const StringName& scheme)
{
    std::unique_lock lock(mutex);
    mount_points.erase(scheme);
}

Optional<std::filesystem::path> PathResolver::Resolve(const VPath& virtual_path, bool check_existence) const
{
    if (!virtual_path.IsValid() || !virtual_path.HasScheme())
    {
        return std::nullopt; // 스키마가 없으면 해석 불가
    }

    std::shared_lock lock(mutex);

    const std::string_view scheme = virtual_path.GetScheme();
    const auto it = mount_points.find(scheme);

    if (it == mount_points.end() || it->second.IsEmpty())
    {
        ConsoleLog(ELogLevel::Warning, "Scheme '{}' is not mounted.", scheme);
        return std::nullopt;
    }

    // 경로 부분에서 맨 앞의 '/' 제거
    std::string_view path_part = virtual_path.GetPathPart();
    if (path_part.starts_with('/'))
    {
        path_part.remove_prefix(1);
    }

    // 모든 마운트 포인트를 순회하며 파일이 실제로 존재하는지 확인 (Mod Fallback)
    for (const MountPoint& mount_point : it->second)
    {
        const std::filesystem::path resolved_path = mount_point.physical_path / path_part;
        if (std::filesystem::exists(resolved_path))
        {
            return resolved_path;
        }
    }

    if (!check_existence)
    {
        // 가장 우선순위가 높은(첫 번째) 마운트 포인트를 사용하여 경로를 조합
        const MountPoint& primary_mount_point = *it->second.Front();
        return primary_mount_point.physical_path / path_part;
    }

    ConsoleLog(ELogLevel::Warning, "File '{}' not found in any mounted path for scheme '{}'.", virtual_path.ToString(), scheme);
    return std::nullopt;
}

Optional<VPath> PathResolver::Unresolve(const std::filesystem::path& physical_path) const
{
    std::shared_lock lock(mutex);

    const auto normalized_physical_path = std::filesystem::absolute(physical_path);

    Optional<VPath> best_match_opt = std::nullopt;
    int best_priority = -1;
    usize longest_match_len = 0;

    for (const auto& [scheme, points] : mount_points)
    {
        for (const MountPoint& point : points)
        {
            // 물리적 경로가 마운트 포인트의 하위 경로인지 확인
            if (normalized_physical_path.native().starts_with(point.physical_path.native())) // 접두사가 일치하는 경우
            {
                const usize match_len = point.physical_path.native().length();

                // 가장 길게 일치하거나, 길이가 같으면 우선순위가 높은 쪽을 선택
                if (match_len > longest_match_len || (match_len == longest_match_len && point.priority > best_priority))
                {
                    using string::ToString;

                    longest_match_len = match_len;
                    best_priority = point.priority;

                    auto relative_part = std::filesystem::relative(normalized_physical_path, point.physical_path);
                    best_match_opt.Emplace(
                        String::Format("{}://{}", scheme.ToString(), relative_part.generic_string())
                    );
                }
            }
        }
    }
    return best_match_opt;
}
}
