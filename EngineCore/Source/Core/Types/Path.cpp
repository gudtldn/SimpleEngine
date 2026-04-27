#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"
#include "SimpleEngine/Core/Serialization/Archive.h"

#include "SDL3/SDL_filesystem.h"


namespace se
{
namespace
{
bool IsAsciiAlpha(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
} // namespace

Path::Path(const char* in_path)
    : path(in_path ? NormalizePath(StringView{ in_path }) : String{})
{
}

Path::Path(const String& in_path)
    : path(in_path.IsEmpty() ? String{} : NormalizePath(StringView{ in_path }))
{
}

Path::Path(StringView in_path)
    : path(in_path.IsEmpty() ? String{} : NormalizePath(in_path))
{
}

Path& Path::Append(const Path& other)
{
    if (other.IsEmpty())
    {
        return *this;
    }

    // 절대 경로가 RHS이면 대체 (std::filesystem::path 동작과 동일)
    if (other.IsAbsolute())
    {
        path = other.path;
        return *this;
    }

    if (path.IsEmpty())
    {
        path = other.path;
        return *this;
    }

    // TODO: [Performance] other에 ".."이 없으면 재정규화 없이 직접 연결하는 fast path 도입 검토
    //       대부분의 Append 호출에서 other는 단순 세그먼트(예: "Source")이므로
    //       NormalizePath O(n) 재호출을 피하면 hot-path에서 유의미한 개선 가능

    // "C:" 형태의 드라이브-상대 루트는 '/' 삽입 시 의미가 바뀌므로 생략
    // C:foo (드라이브-상대) vs C:/foo (드라이브-절대)
    const bool is_drive_relative_root = (path.ByteLen() == 2 && DetectRootLength(path) == 2);

    String combined;
    combined.Reserve(path.ByteLen() + (is_drive_relative_root ? 0 : 1) + other.path.ByteLen());
    combined.Append(path);

    if (!is_drive_relative_root)
    {
        combined += '/';
    }

    combined.Append(other.path);
    path = NormalizePath(combined);
    return *this;
}

Path& Path::operator/=(const Path& other)
{
    return Append(other);
}

Path& Path::Concat(const String& str)
{
    if (str.IsEmpty())
    {
        return *this;
    }

    // TODO: [Performance] str에 경로 구분자('/', '\\')나 ".."이 없으면 재정규화 생략 가능
    //       확장자 변경 등 단순 연결 시 NormalizePath 비용 회피
    String combined;
    combined.Reserve(path.ByteLen() + str.ByteLen());
    combined.Append(path);
    combined.Append(str);
    path = NormalizePath(combined);
    return *this;
}

Path& Path::operator+=(const String& str)
{
    return Concat(str);
}

Path& Path::SetFileName(const String& name)
{
    const StringView view{ path };
    const usize root_len = DetectRootLength(view);

    const Optional<usize> last_slash = view.FindLast('/');
    if (last_slash.HasValue() && *last_slash >= root_len)
    {
        path.Truncate(*last_slash + 1);
    }
    else
    {
        path.Clear();
    }

    path.Append(name);
    path = NormalizePath(path);
    return *this;
}

Path& Path::SetExtension(const String& extension)
{
    // 파일명 내에서 확장자 위치를 찾아 교체
    const StringView view{ path };
    const Optional<usize> last_slash = view.FindLast('/');
    const usize name_start = last_slash.HasValue() ? (*last_slash + 1) : 0;
    const StringView name_part = view.Substr(name_start);

    // 이름 부분에서 마지막 '.' 찾기 (첫 글자 '.'는 숨김파일이므로 제외)
    const Optional<usize> dot = name_part.FindLast('.');
    if (dot.HasValue() && *dot > 0)
    {
        path.Truncate(name_start + *dot);
    }

    // 새 확장자 추가
    if (!extension.IsEmpty())
    {
        if (!StringView{ extension }.StartsWith('.'))
        {
            path += ".";
        }
        path.Append(extension);
    }

    // 확장자를 통한 Path Traversal 방지(예: ".ext/../") 및 정규화 불변식 유지
    path = NormalizePath(path);

    return *this;
}

Path Path::operator/(const Path& other) const
{
    Path new_path = *this;
    new_path.Append(other);
    return new_path;
}

Path Path::WithFileName(const String& name) const
{
    Path new_path = *this;
    new_path.SetFileName(name);
    return new_path;
}

Path Path::WithExtension(const String& extension) const
{
    Path new_path = *this;
    new_path.SetExtension(extension);
    return new_path;
}

Optional<Path> Path::RelativeTo(const Path& base) const
{
    // 절대/상대 불일치 시 계산 불가
    if (IsAbsolute() != base.IsAbsolute())
    {
        return NullOpt;
    }

    const StringView this_view{ path };
    const StringView base_view{ base.path };
    const usize this_root = DetectRootLength(this_view);
    const usize base_root = DetectRootLength(base_view);

    // 루트 접두사가 다르면 계산 불가
    if (this_view.Substr(0, this_root) != base_view.Substr(0, base_root))
    {
        return NullOpt;
    }

    // 세그먼트 분할 헬퍼
    auto split = [](StringView view, usize root_len) -> Array<StringView>
    {
        Array<StringView> segments;
        usize pos = root_len;
        while (pos < view.ByteLen())
        {
            const Optional<usize> slash = view.Find('/', pos);
            const usize end = slash.HasValue() ? *slash : view.ByteLen();
            if (end > pos)
            {
                segments.Push(view.Substr(pos, end - pos));
            }
            pos = end + 1;
        }
        return segments;
    };

    const Array<StringView> this_segs = split(this_view, this_root);
    const Array<StringView> base_segs = split(base_view, base_root);

    // 공통 접두사 길이
    usize common = 0;
    while (
        common < this_segs.Len() && common < base_segs.Len()
        && this_segs[common] == base_segs[common]
    )
    {
        ++common;
    }

    // 공통 부분이 없고 양쪽 모두 비어있지 않으면 관계 없음
    if (common == 0 && !this_segs.IsEmpty() && !base_segs.IsEmpty())
    {
        return NullOpt;
    }

    // 결과 조립: base 잔여분만큼 ".." + this 잔여분
    String result;
    for (usize i = common; i < base_segs.Len(); ++i)
    {
        if (!result.IsEmpty())
        {
            result += '/';
        }
        result += "..";
    }
    for (usize i = common; i < this_segs.Len(); ++i)
    {
        if (!result.IsEmpty())
        {
            result += '/';
        }
        result.Append(this_segs[i]);
    }

    if (result.IsEmpty())
    {
        result = ".";
    }

    Path rel;
    rel.path = std::move(result);
    return rel;
}

Optional<Path> Path::Parent() const
{
    if (path.IsEmpty())
    {
        return NullOpt;
    }

    const StringView view{ path };
    const usize root_len = DetectRootLength(view);

    const Optional<usize> last_slash = view.FindLast('/');
    if (!last_slash.HasValue())
    {
        // 슬래시 없음 (예: "foo") -> 부모 없음
        return NullOpt;
    }

    if (*last_slash < root_len)
    {
        // 슬래시가 루트 접두사 내에 있음 (예: "C:/foo"의 "/" 또는 "/"의 "/")
        // 현재 경로가 루트 자체인 경우
        if (view.ByteLen() <= root_len)
        {
            return NullOpt;
        }
        // 루트 바로 아래의 항목 -> 루트를 반환
        Path parent;
        parent.path = view.Substr(0, root_len).ToString();
        return parent;
    }

    // 일반적인 경우: 마지막 슬래시 이전까지
    Path parent;
    parent.path = view.Substr(0, *last_slash).ToString();
    return parent;
}

Optional<String> Path::FileName() const
{
    if (path.IsEmpty())
    {
        return NullOpt;
    }

    const StringView view{ path };
    const usize root_len = DetectRootLength(view);

    // 경로가 루트 자체인 경우
    if (view.ByteLen() <= root_len)
    {
        return NullOpt;
    }

    const Optional<usize> last_slash = view.FindLast('/');
    StringView name = last_slash.HasValue() ? view.Substr(*last_slash + 1) : view;

    if (name.IsEmpty())
    {
        return NullOpt;
    }
    return name.ToString();
}

Optional<String> Path::FileStem() const
{
    Optional<String> name = FileName();
    if (!name.HasValue())
    {
        return NullOpt;
    }

    const StringView view{ *name };
    const Optional<usize> dot = view.FindLast('.');

    // dot이 없거나 첫 글자가 '.'인 경우 (숨김파일, 전체가 stem)
    if (!dot.HasValue() || *dot == 0)
    {
        return name;
    }

    return view.Substr(0, *dot).ToString();
}

Optional<String> Path::Extension() const
{
    const Optional<String> name = FileName();
    if (!name.HasValue())
    {
        return NullOpt;
    }

    const StringView view{ *name };
    const Optional<usize> dot = view.FindLast('.');

    if (!dot.HasValue() || *dot == 0)
    {
        return NullOpt;
    }

    return view.Substr(*dot).ToString();
}

bool Path::IsEmpty() const
{
    return path.IsEmpty();
}

bool Path::IsAbsolute() const
{
    const StringView view{ path };
    if (view.IsEmpty())
    {
        return false;
    }

    // Unix 절대 경로
    if (view[0] == '/')
    {
        return true;
    }

    // Windows 절대 경로: C:/
    if (view.ByteLen() >= 3 && IsAsciiAlpha(view[0]) && view[1] == ':' && view[2] == '/')
    {
        return true;
    }

    return false;
}

bool Path::IsRelative() const
{
    return !IsAbsolute();
}

bool Path::IsSubPathOf(const Path& base) const
{
    const Optional<Path> rel = RelativeTo(base);
    if (!rel.HasValue())
    {
        return false;
    }

    const StringView view{ rel->path };
    return !(view == ".." || view.StartsWith("../"));
}

bool Path::Exists() const
{
    return FileSystem::Exists(*this);
}

bool Path::IsDirectory() const
{
    if (path.IsEmpty())
    {
        return false;
    }
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path.CStr(), &info))
    {
        return false;
    }
    return info.type == SDL_PATHTYPE_DIRECTORY;
}

bool Path::IsFile() const
{
    if (path.IsEmpty())
    {
        return false;
    }
    SDL_PathInfo info;
    if (!SDL_GetPathInfo(path.CStr(), &info))
    {
        return false;
    }
    return info.type == SDL_PATHTYPE_FILE;
}

const String& Path::ToString() const
{
    return path;
}

const char* Path::CStr() const
{
    return path.CStr();
}

void Path::Swap(Path& other) noexcept
{
    std::swap(path, other.path);
}

bool Path::operator==(const Path& other) const
{
    return path == other.path;
}

std::strong_ordering Path::operator<=>(const Path& other) const
{
    return StringView{ path } <=> StringView{ other.path };
}

String Path::NormalizePath(StringView input)
{
    if (input.IsEmpty())
    {
        return {};
    }

    const usize len = input.ByteLen();

    // 작업 버퍼 할당 (최악의 경우 원본과 동일 길이)
    String buffer;
    buffer.ResizeForOverwrite(len);
    char* buf = buffer.Data();

    // Step 1: 복사하면서 '\' -> '/'
    for (usize i = 0; i < len; ++i)
    {
        buf[i] = (input[i] == '\\') ? '/' : input[i];
    }

    // Step 2: 루트 접두사 탐지
    const usize root_len = DetectRootLength(StringView{ buf, len });
    const bool is_absolute = (root_len > 0 && buf[0] == '/')
        || (root_len >= 3 && IsAsciiAlpha(buf[0]) && buf[1] == ':' && buf[2] == '/');

    // Step 3: 세그먼트 처리 (in-place write-pointer)
    usize write = root_len;
    usize read = root_len;

    // 세그먼트 시작 위치 스택 (일반적인 경로 깊이 ≤ 32)
    Array<usize> seg_starts;

    while (read < len)
    {
        // 연속 슬래시 skip
        while (read < len && buf[read] == '/')
        {
            ++read;
        }
        if (read >= len)
        {
            break;
        }

        // 세그먼트 추출
        const usize seg_begin = read;
        while (read < len && buf[read] != '/')
        {
            ++read;
        }
        const usize seg_len = read - seg_begin;

        // "." 세그먼트: skip
        if (seg_len == 1 && buf[seg_begin] == '.')
        {
            continue;
        }

        // ".." 세그먼트
        if (seg_len == 2 && buf[seg_begin] == '.' && buf[seg_begin + 1] == '.')
        {
            if (!seg_starts.IsEmpty())
            {
                const usize prev_start = seg_starts.Back().Value();
                const usize prev_len = write - prev_start;
                const bool prev_is_dotdot = (prev_len == 2 && buf[prev_start] == '.' && buf[prev_start + 1] == '.');

                if (!prev_is_dotdot)
                {
                    // 이전 세그먼트를 pop (구분자 포함)
                    write = prev_start;
                    seg_starts.Pop();
                    if (write > root_len && write > 0 && buf[write - 1] == '/')
                    {
                        --write;
                    }
                    continue;
                }
            }

            if (!is_absolute)
            {
                // 상대 경로: ".."을 그대로 출력
                if (write > root_len)
                {
                    buf[write++] = '/';
                }
                seg_starts.Push(write);
                buf[write++] = '.';
                buf[write++] = '.';
            }
            // 절대 경로에서 루트 위로의 ..는 무시
            continue;
        }

        // 일반 세그먼트 - UNC 루트("//server/share")는 '/'로 끝나지 않으므로 구분자 보충
        if (write > root_len || (write == root_len && root_len >= 2 && buf[0] == '/' && buf[1] == '/'))
        {
            buf[write++] = '/';
        }
        seg_starts.Push(write);
        if (write != seg_begin)
        {
            std::memmove(buf + write, buf + seg_begin, seg_len);
        }
        write += seg_len;
    }

    // 축퇴 처리: 상대 경로가 빈 문자열로 해소된 경우
    if (write == 0)
    {
        return ".";
    }

    buffer.Truncate(write);
    return buffer;
}

usize Path::DetectRootLength(StringView view)
{
    const usize len = view.ByteLen();
    if (len == 0)
    {
        return 0;
    }

    // Windows 드라이브 문자: "C:" 또는 "C:/"
    if (len >= 2 && IsAsciiAlpha(view[0]) && view[1] == ':')
    {
        if (len >= 3 && view[2] == '/')
        {
            return 3;
        }
        return 2;
    }

    // UNC 경로: "//server/share"
    if (len >= 2 && view[0] == '/' && view[1] == '/')
    {
        usize pos = 2;
        while (pos < len && view[pos] != '/')
        {
            ++pos;  // server
        }
        if (pos < len)
        {
            ++pos;  // separator
        }
        while (pos < len && view[pos] != '/')
        {
            ++pos;  // share
        }
        return pos;
    }

    // Unix 절대 경로: "/"
    if (view[0] == '/')
    {
        return 1;
    }

    return 0;
}

void SerializeInline(Archive& ar, Path& path)
{
    String str = path.path;
    ar << str;
    if (ar.IsLoading())
    {
        path.path = Path::NormalizePath(str);
    }
}
} // namespace se
