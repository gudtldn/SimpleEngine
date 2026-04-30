#include "SimpleEngine/Core/HAL/FileDialog.h"
#include "SimpleEngine/Core/Logging/Logging.h"

#include <atomic>
#include <variant>


namespace se
{
namespace
{
std::atomic<bool> IsDialogOpen = false;

// SDL 콜백에 사용자 데이터를 전달하기 위한 프록시 구조체
struct DialogCallbackProxy
{
    std::variant<
        FileDialog::OnFileSelected,
        FileDialog::OnMultiFilesSelected
    > callback;

    // SDL C-style 콜백 함수
    static void SDLCALL Callback(void* userdata, const char* const* file_list, [[maybe_unused]] int filter)
    {
        IsDialogOpen = false;

        auto* proxy = static_cast<DialogCallbackProxy*>(userdata);
        if (file_list)
        {
            std::visit([&]<typename Fn>(Fn&& cb)
            {
                using DecayedFn = std::decay_t<Fn>;

                // 단일 선택
                if constexpr (std::same_as<DecayedFn, FileDialog::OnFileSelected>)
                {
                    // file_list[0]이 존재하면 호출
                    if (file_list[0] != nullptr)
                    {
                        std::forward<Fn>(cb)(Path(file_list[0]));
                    }
                }

                // 다중 선택
                else if constexpr (std::same_as<DecayedFn, FileDialog::OnMultiFilesSelected>)
                {
                    Array<Path> paths;
                    for (int i = 0; file_list[i] != nullptr; ++i)
                    {
                        paths.Emplace(file_list[i]);
                    }

                    // 선택된 파일이 있을 때만 호출 (취소 시 호출 안 함)
                    if (!paths.IsEmpty())
                    {
                        std::forward<Fn>(cb)(paths);
                    }
                }
            }, proxy->callback);
        }

        // 힙에 할당된 프록시 객체 삭제
        delete proxy;
    }
};

// SDL 필터 구조체로 변환하는 헬퍼
Array<SDL_DialogFileFilter> ConvertFilters(ArrayView<const FileFilter> filters)
{
    Array<SDL_DialogFileFilter> sdl_filters;
    sdl_filters.Reserve(filters.Len());
    for (const auto& [name, pattern] : filters)
    {
        sdl_filters.Push({
            .name = name,
            .pattern = pattern,
        });
    }
    return sdl_filters;
}

// 다이얼로그 열기 시도
[[nodiscard]] bool TryOpenDialog()
{
    bool expected = false;
    if (IsDialogOpen.compare_exchange_strong(expected, true))
    {
        return true;
    }

    ConsoleLog(ELogLevel::Warning, "File dialog is already open. Ignoring request.");
    return false;
}
} // namespace

void FileDialog::OpenFile(OnFileSelected callback, ArrayView<const FileFilter> filters, const char* default_location, SDL_Window* window)
{
    if (!TryOpenDialog())
    {
        return;
    }

    auto* proxy = new DialogCallbackProxy{ std::move(callback) };
    auto sdl_filters = ConvertFilters(filters);

    SDL_ShowOpenFileDialog(
        DialogCallbackProxy::Callback,
        proxy,
        window,
        sdl_filters.Data(),
        static_cast<int>(sdl_filters.Len()),
        default_location,
        false // allow_many = false
    );
}

void FileDialog::OpenFiles(OnMultiFilesSelected callback, ArrayView<const FileFilter> filters, const char* default_location, SDL_Window* window)
{
    if (!TryOpenDialog())
    {
        return;
    }

    auto* proxy = new DialogCallbackProxy{ std::move(callback) };
    auto sdl_filters = ConvertFilters(filters);

    SDL_ShowOpenFileDialog(
        DialogCallbackProxy::Callback,
        proxy,
        window,
        sdl_filters.Data(),
        static_cast<int>(sdl_filters.Len()),
        default_location,
        true // allow_many = true
    );
}

void FileDialog::SaveFile(OnFileSelected callback, ArrayView<const FileFilter> filters, const char* default_location, SDL_Window* window)
{
    if (!TryOpenDialog())
    {
        return;
    }

    // 첫 번째 필터의 확장자를 추출하여 자동 부여 래핑
    // pattern은 "fbx" 또는 "png;jpg" 형태 (세미콜론 앞이 기본 확장자)
    String default_ext = [&] -> String
    {
        if (filters.IsEmpty())
        {
            return {};
        }

        const StringView pattern{ filters[0].pattern };
        const Optional<usize> sep = pattern.Find(';');

        return sep.Map([&](usize idx) -> String
        {
            return pattern.Substr(0, idx);
        })
        .ValueOr(pattern);
    }();

    auto wrapped = [cb = std::move(callback), ext = std::move(default_ext)](const Path& path)
    {
        if (!ext.IsEmpty() && !path.Extension().HasValue())
        {
            Path with_ext = path;
            with_ext.SetExtension(ext);
            cb(with_ext);
        }
        else
        {
            cb(path);
        }
    };

    auto* proxy = new DialogCallbackProxy{ OnFileSelected{ std::move(wrapped) } };
    auto sdl_filters = ConvertFilters(filters);

    SDL_ShowSaveFileDialog(
        DialogCallbackProxy::Callback,
        proxy,
        window,
        sdl_filters.Data(),
        static_cast<int>(sdl_filters.Len()),
        default_location
    );
}
} // namespace se
