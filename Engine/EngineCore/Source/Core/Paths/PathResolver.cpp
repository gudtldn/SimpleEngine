module SE.Core;
import :Paths.PathResolver;

import SE.Core;
import SE.Utility;

import <cassert>;


namespace se::core::paths
{
PathResolver* PathResolver::Instance = nullptr;

PathResolver& PathResolver::Get()
{
    if (!Instance)
    {
        Instance = new PathResolver;
    }
    return *Instance;
}

void PathResolver::Mount(std::u8string_view scheme, const std::filesystem::path& physical_path, int priority)
{
    // 경로를 정규화하여 저장
    auto normalized_path = std::filesystem::absolute(physical_path);

    std::vector<MountPoint>& points = mount_points[se::u8string(scheme)];
    points.push_back({
        .physical_path = std::move(normalized_path),
        .priority = priority
    });

    // 우선순위가 높은 것이 앞에 오도록 정렬 (stable_sort로 순서 유지)
    std::ranges::stable_sort(points, std::greater{});
}

void PathResolver::Unmount(std::u8string_view scheme)
{
    mount_points.erase(se::u8string(scheme));
}

Optional<std::filesystem::path> PathResolver::Resolve(const VPath& virtual_path) const
{
    if (!virtual_path.IsValid() || !virtual_path.HasScheme())
    {
        return std::nullopt; // 스키마가 없으면 해석 불가
    }

    const auto scheme = virtual_path.GetScheme();
    const auto it = mount_points.find(se::u8string(scheme));

    if (it == mount_points.end() || it->second.empty())
    {
        ConsoleLog(ELogLevel::Warning, u8"Scheme '{}' is not mounted.", scheme);
        return std::nullopt;
    }

    // 경로 부분에서 맨 앞의 '/' 제거
    auto path_part = virtual_path.GetPathPart();
    if (path_part.starts_with(u8'/'))
    {
        path_part.remove_prefix(1);
    }

    // 우선순위가 가장 높은 마운트 포인트(0번 인덱스)를 사용
    // TODO: 모든 마운트 포인트를 순회하며 파일이 실제로 존재하는지 확인할 수도 있음 (Mod Fallback)
    const auto& base_path = it->second[0].physical_path;
    return base_path / path_part;
}

Optional<VPath> PathResolver::Unresolve(const std::filesystem::path& physical_path) const
{
    const auto normalized_physical_path = std::filesystem::absolute(physical_path);

    Optional<VPath> best_match_opt = std::nullopt;
    int best_priority = -1;
    size_t longest_match_len = 0;

    for (const auto& [scheme, points] : mount_points)
    {
        for (const MountPoint& point : points)
        {
            // 물리적 경로가 마운트 포인트의 하위 경로인지 확인
            if (normalized_physical_path.native().starts_with(point.physical_path.native())) // 접두사가 일치하는 경우
            {
                const size_t match_len = point.physical_path.native().length();

                // 가장 길게 일치하거나, 길이가 같으면 우선순위가 높은 쪽을 선택
                if (match_len > longest_match_len || (match_len == longest_match_len && point.priority > best_priority))
                {
                    using utility::string_utils::ToU8String;

                    longest_match_len = match_len;
                    best_priority = point.priority;

                    auto relative_part = std::filesystem::relative(normalized_physical_path, point.physical_path);
                    best_match_opt.Emplace(
                        ToU8String(std::format("{}://{}", scheme, relative_part.generic_u8string()))
                    );
                }
            }
        }
    }
    return best_match_opt;
}
}
