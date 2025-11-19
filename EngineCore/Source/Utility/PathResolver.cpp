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

void PathResolver::Mount(const StringName& scheme, const std::filesystem::path& physical_path, int32 priority)
{
    std::unique_lock lock(mutex);

    // 경로를 정규화하여 저장
    auto abs_path = std::filesystem::absolute(physical_path);
    abs_path.make_preferred();

    Array<MountPoint>& points = mount_points[scheme];
    const auto it = std::ranges::find_if(points, [&](const MountPoint& point)
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
}

void PathResolver::Unmount(const StringName& scheme)
{
    std::unique_lock lock(mutex);
    mount_points.Remove(scheme);
}

Optional<std::filesystem::path> PathResolver::Resolve(const VPath& virtual_path, bool check_existence) const
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
        std::filesystem::path candidate = mount_point.physical_path / relative_part;

        if (check_existence)
        {
            std::error_code ec;
            if (std::filesystem::exists(candidate, ec) && !ec)
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

Optional<VPath> PathResolver::Unresolve(const std::filesystem::path& physical_path) const
{
    std::shared_lock lock(mutex);

    std::filesystem::path abs_input = std::filesystem::absolute(physical_path);
    abs_input.make_preferred();
    const auto& input_str = abs_input.native();

    const StringName* best_scheme = nullptr;
    const MountPoint* best_mount_point = nullptr;
    usize best_match_len = 0;

    for (const auto& [scheme, points] : mount_points)
    {
        for (const MountPoint& point : points)
        {
            const auto& root_str = point.physical_path.native();

            // 물리적 경로가 마운트 포인트의 하위 경로인지 확인
            if (input_str.starts_with(root_str)) // 접두사가 일치하는 경우
            {
                if (
                    input_str.size() == root_str.size()
                    || input_str[root_str.size()] == std::filesystem::path::preferred_separator
                ) {
                    // 더 긴 경로가 매칭되거나 (하위 폴더 마운트 우선), 길이는 같은데 우선순위가 높은 경우 선택
                    if (
                        root_str.size() > best_match_len
                        || (root_str.size() == best_match_len && (!best_mount_point || point.priority > best_mount_point->priority))
                    ) {
                        best_match_len = root_str.size();
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
        const std::filesystem::path relative = std::filesystem::relative(abs_input, best_mount_point->physical_path);
        std::string generic_rel = relative.generic_string();

        return VPath{ String::Format("{}://{}", best_scheme->ToString(), generic_rel) };
    }

    return std::nullopt;
}
}
