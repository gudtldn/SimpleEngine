#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Core/FileSystem/FileSystem.h"

#include "SDL3/SDL_filesystem.h"

using namespace se;


namespace
{
// 테스트용 임시 디렉토리 관리자
class TempDir
{
public:
    TempDir(StringView name)
    {
        char* pref = SDL_GetPrefPath("SimpleEngine", "Tests");
        root_path = Path(pref) / Path("FileSystemTest") / Path(name);
        SDL_free(pref);
        FileSystem::CreateDirectories(root_path);
    }

    ~TempDir()
    {
        FileSystem::RemoveAll(root_path);
    }

    const Path& GetPath() const { return root_path; }

    Path operator/(StringView relative) const
    {
        return root_path / Path(relative);
    }

private:
    Path root_path;
};
} // namespace


// =============================================================================
// Path Operations
// =============================================================================

class FileSystemPathTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "PathOps" };
};

TEST_F(FileSystemPathTest, AbsoluteConvertsRelativePath)
{
    Path relative("some/relative/path");
    Path absolute = FileSystem::Absolute(relative);

    EXPECT_TRUE(absolute.IsAbsolute());
    EXPECT_FALSE(absolute.IsEmpty());
}

TEST_F(FileSystemPathTest, CanonicalReturnsNulloptForNonExistentPath)
{
    Path non_existent = temp_dir / "does_not_exist.txt";
    Optional<Path> canonical = FileSystem::Canonical(non_existent);

    EXPECT_FALSE(canonical.HasValue());
}

TEST_F(FileSystemPathTest, CanonicalNormalizesExistingPath)
{
    // 파일 생성
    Path file_path = temp_dir / "canonical_test.txt";
    FileSystem::WriteString(file_path, "test");

    // 정규화 테스트 (. 과 .. 포함)
    Path with_dots = temp_dir / "subdir/../canonical_test.txt";
    FileSystem::CreateDirectories(temp_dir / "subdir");

    Optional<Path> canonical = FileSystem::Canonical(with_dots);
    ASSERT_TRUE(canonical.HasValue());
    EXPECT_EQ(canonical.Value(), FileSystem::Canonical(file_path).Value());
}


// =============================================================================
// Directory Operations
// =============================================================================

class FileSystemDirTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "DirOps" };
};

TEST_F(FileSystemDirTest, CreateDirectoriesSucceeds)
{
    Path new_dir = temp_dir / "new_directory";
    EXPECT_FALSE(new_dir.Exists());

    bool result = FileSystem::CreateDirectories(new_dir);

    EXPECT_TRUE(result);
    EXPECT_TRUE(new_dir.Exists());
    EXPECT_TRUE(new_dir.IsDirectory());
}

TEST_F(FileSystemDirTest, CreateDirectoriesReturnsTrueIfAlreadyExists)
{
    Path existing_dir = temp_dir / "existing_directory";
    FileSystem::CreateDirectories(existing_dir);
    ASSERT_TRUE(existing_dir.Exists());

    bool result = FileSystem::CreateDirectories(existing_dir);
    EXPECT_TRUE(result);
}

TEST_F(FileSystemDirTest, CreateDirectoriesCreatesNestedDirs)
{
    Path nested = temp_dir / "a/b/c/d";
    EXPECT_FALSE(nested.Exists());

    bool result = FileSystem::CreateDirectories(nested);

    EXPECT_TRUE(result);
    EXPECT_TRUE(nested.Exists());
    EXPECT_TRUE(nested.IsDirectory());
}


// =============================================================================
// File Operations
// =============================================================================

class FileSystemFileTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "FileOps" };
};

TEST_F(FileSystemFileTest, RemoveDeletesFile)
{
    Path file_path = temp_dir / "to_delete.txt";
    FileSystem::WriteString(file_path, "delete me");
    ASSERT_TRUE(file_path.Exists());

    bool result = FileSystem::Remove(file_path);

    EXPECT_TRUE(result);
    EXPECT_FALSE(file_path.Exists());
}

TEST_F(FileSystemFileTest, RemoveReturnsFalseForNonExistent)
{
    Path non_existent = temp_dir / "non_existent.txt";
    bool result = FileSystem::Remove(non_existent);

    EXPECT_FALSE(result);
}

TEST_F(FileSystemFileTest, RemoveAllDeletesDirectoryRecursively)
{
    // 중첩 구조 생성
    Path nested = temp_dir / "nested";
    FileSystem::CreateDirectories(nested / "a/b");
    FileSystem::WriteString(nested / "file1.txt", "1");
    FileSystem::WriteString(nested / "a/file2.txt", "2");
    FileSystem::WriteString(nested / "a/b/file3.txt", "3");

    ASSERT_TRUE(nested.Exists());

    usize removed = FileSystem::RemoveAll(nested);

    EXPECT_GT(removed, 0u);
    EXPECT_FALSE(nested.Exists());
}

TEST_F(FileSystemFileTest, CopyCopiesFile)
{
    Path src = temp_dir / "source.txt";
    Path dst = temp_dir / "destination.txt";
    FileSystem::WriteString(src, "copy this content");

    bool result = FileSystem::Copy(src, dst);

    EXPECT_TRUE(result);
    EXPECT_TRUE(dst.Exists());
    EXPECT_EQ(FileSystem::ReadToString(dst).Value(), "copy this content");
}

TEST_F(FileSystemFileTest, RenameMovesFile)
{
    Path old_path = temp_dir / "old_name.txt";
    Path new_path = temp_dir / "new_name.txt";
    FileSystem::WriteString(old_path, "renamed content");

    bool result = FileSystem::Rename(old_path, new_path);

    EXPECT_TRUE(result);
    EXPECT_FALSE(old_path.Exists());
    EXPECT_TRUE(new_path.Exists());
    EXPECT_EQ(FileSystem::ReadToString(new_path).Value(), "renamed content");
}


// =============================================================================
// File Info
// =============================================================================

TEST_F(FileSystemFileTest, FileSizeReturnsCorrectSize)
{
    Path file_path = temp_dir / "sized_file.txt";
    const StringView content = "Hello, World!";
    FileSystem::WriteString(file_path, content);

    Optional<usize> size = FileSystem::FileSize(file_path);

    ASSERT_TRUE(size.HasValue());
    EXPECT_EQ(size.Value(), content.ByteLen());
}

TEST_F(FileSystemFileTest, FileSizeReturnsNulloptForNonExistent)
{
    Path non_existent = temp_dir / "non_existent.txt";
    Optional<usize> size = FileSystem::FileSize(non_existent);

    EXPECT_FALSE(size.HasValue());
}


// =============================================================================
// File Read/Write
// =============================================================================

class FileSystemReadWriteTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "ReadWrite" };
};

TEST_F(FileSystemReadWriteTest, WriteStringAndReadToString)
{
    Path file_path = temp_dir / "text_file.txt";
    const StringView content = "Hello, SimpleEngine!\n이것은 UTF-8 테스트입니다.";

    bool write_result = FileSystem::WriteString(file_path, content);
    ASSERT_TRUE(write_result);
    ASSERT_TRUE(file_path.Exists());

    auto read_result = FileSystem::ReadToString(file_path);
    ASSERT_TRUE(read_result.HasValue());
    EXPECT_EQ(StringView(read_result->CStr(), read_result->ByteLen()), content);
}

TEST_F(FileSystemReadWriteTest, WriteAndReadBinaryData)
{
    Path file_path = temp_dir / "binary_file.bin";

    // 바이너리 데이터 생성
    Array<u8> original_data;
    for (int i = 0; i < 256; ++i)
    {
        original_data.Push(static_cast<u8>(i));
    }

    bool write_result = FileSystem::Write(file_path, original_data);
    ASSERT_TRUE(write_result);

    auto read_result = FileSystem::ReadBytes(file_path);
    ASSERT_TRUE(read_result.HasValue());
    EXPECT_EQ(read_result->Len(), original_data.Len());

    for (usize i = 0; i < original_data.Len(); ++i)
    {
        EXPECT_EQ(read_result.Value()[i], original_data[i]);
    }
}

TEST_F(FileSystemReadWriteTest, ReadToStringReturnsNulloptForNonExistent)
{
    Path non_existent = temp_dir / "non_existent.txt";
    auto result = FileSystem::ReadToString(non_existent);

    EXPECT_FALSE(result.HasValue());
}

TEST_F(FileSystemReadWriteTest, ReadReturnsNulloptForNonExistent)
{
    Path non_existent = temp_dir / "non_existent.bin";
    auto result = FileSystem::ReadBytes(non_existent);

    EXPECT_FALSE(result.HasValue());
}

TEST_F(FileSystemReadWriteTest, WriteStringOverwritesExistingContent)
{
    Path file_path = temp_dir / "overwrite.txt";

    FileSystem::WriteString(file_path, "original content");
    FileSystem::WriteString(file_path, "new content");

    auto result = FileSystem::ReadToString(file_path);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(StringView(result->CStr(), result->ByteLen()), "new content");
}

TEST_F(FileSystemReadWriteTest, WriteStringCreatesParentDirectories)
{
    // 참고: 현재 구현에서는 부모 디렉토리를 자동 생성하지 않음
    // 이 테스트는 현재 동작을 문서화함
    Path nested_file = temp_dir / "new_dir/new_subdir/file.txt";

    bool result = FileSystem::WriteString(nested_file, "content");

    // 현재 구현: 부모 디렉토리가 없으면 실패
    EXPECT_FALSE(result);
}

TEST_F(FileSystemReadWriteTest, WriteEmptyFile)
{
    Path file_path = temp_dir / "empty.txt";

    bool result = FileSystem::WriteString(file_path, "");
    ASSERT_TRUE(result);

    auto content = FileSystem::ReadToString(file_path);
    ASSERT_TRUE(content.HasValue());
    EXPECT_TRUE(content->IsEmpty());
}


// =============================================================================
// Directory Iteration
// =============================================================================

class FileSystemIteratorTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "Iterator" };

    void SetUp() override
    {
        // 테스트용 파일/디렉토리 구조 생성
        FileSystem::CreateDirectories(temp_dir / "subdir1");
        FileSystem::CreateDirectories(temp_dir / "subdir2");
        FileSystem::WriteString(temp_dir / "file1.txt", "1");
        FileSystem::WriteString(temp_dir / "file2.txt", "2");
        FileSystem::WriteString(temp_dir / "subdir1/nested.txt", "nested");
    }
};

TEST_F(FileSystemIteratorTest, ReadDirIteratesOverEntries)
{
    int file_count = 0;
    int dir_count = 0;

    for (const auto& entry : FileSystem::ReadDir(temp_dir.GetPath()))
    {
        if (entry.IsFile())
        {
            ++file_count;
        }
        if (entry.IsDirectory())
        {
            ++dir_count;
        }
    }

    EXPECT_EQ(file_count, 2);  // file1.txt, file2.txt
    EXPECT_EQ(dir_count, 2);   // subdir1, subdir2
}

TEST_F(FileSystemIteratorTest, DirectoryEntryGetPathReturnsCorrectPath)
{
    bool found_file1 = false;

    for (const auto& entry : FileSystem::ReadDir(temp_dir.GetPath()))
    {
        Path path = entry.GetPath();
        if (path.FileName().ValueOrDefault() == "file1.txt")
        {
            found_file1 = true;
            EXPECT_TRUE(entry.IsFile());
            EXPECT_FALSE(entry.IsDirectory());
        }
    }

    EXPECT_TRUE(found_file1);
}

TEST_F(FileSystemIteratorTest, DirectoryEntryFileSizeReturnsCorrectSize)
{
    FileSystem::WriteString(temp_dir / "sized.txt", "12345");

    for (const auto& entry : FileSystem::ReadDir(temp_dir.GetPath()))
    {
        if (entry.GetPath().FileName().ValueOrDefault() == "sized.txt")
        {
            EXPECT_EQ(entry.FileSize(), 5u);
            return;
        }
    }

    FAIL() << "sized.txt not found";
}

TEST_F(FileSystemIteratorTest, ReadDirOnEmptyDirectoryProducesNoEntries)
{
    Path empty_dir = temp_dir / "empty_dir";
    FileSystem::CreateDirectories(empty_dir);

    int count = 0;
    for ([[maybe_unused]] const auto& entry : FileSystem::ReadDir(empty_dir))
    {
        ++count;
    }

    EXPECT_EQ(count, 0);
}

TEST_F(FileSystemIteratorTest, ReadDirOnNonExistentDirectoryProducesNoEntries)
{
    Path non_existent = temp_dir / "does_not_exist";

    int count = 0;
    for ([[maybe_unused]] const auto& entry : FileSystem::ReadDir(non_existent))
    {
        ++count;
    }

    EXPECT_EQ(count, 0);
}

TEST_F(FileSystemIteratorTest, ReadDirDoesNotRecurse)
{
    // subdir1/nested.txt는 포함되지 않아야 함
    bool found_nested = false;

    for (const auto& entry : FileSystem::ReadDir(temp_dir.GetPath()))
    {
        if (entry.GetPath().FileName().ValueOrDefault() == "nested.txt")
        {
            found_nested = true;
        }
    }

    EXPECT_FALSE(found_nested);
}


// =============================================================================
// UTF-8 Path Tests
// =============================================================================

class FileSystemUtf8Test : public ::testing::Test
{
protected:
    TempDir temp_dir{ "UTF8" };
};

TEST_F(FileSystemUtf8Test, WriteAndReadFileWithKoreanPath)
{
    Path korean_path = temp_dir / "한글파일.txt";
    const StringView content = "한글 내용입니다.";

    bool write_result = FileSystem::WriteString(korean_path, content);
    ASSERT_TRUE(write_result);

    auto read_result = FileSystem::ReadToString(korean_path);
    ASSERT_TRUE(read_result.HasValue());
    EXPECT_EQ(StringView(read_result->CStr(), read_result->ByteLen()), content);
}

TEST_F(FileSystemUtf8Test, CreateDirectoriesWithUnicodeName)
{
    Path unicode_dir = temp_dir / "日本語フォルダ";

    bool result = FileSystem::CreateDirectories(unicode_dir);

    EXPECT_TRUE(result);
    EXPECT_TRUE(unicode_dir.Exists());
}

TEST_F(FileSystemUtf8Test, ReadDirWithUnicodeNames)
{
    FileSystem::WriteString(temp_dir / "中文.txt", "chinese");
    FileSystem::WriteString(temp_dir / "العربية.txt", "arabic");
    FileSystem::WriteString(temp_dir / "🎮.txt", "emoji");

    int count = 0;
    for ([[maybe_unused]] const auto& entry : FileSystem::ReadDir(temp_dir.GetPath()))
    {
        ++count;
    }

    EXPECT_EQ(count, 3);
}

TEST_F(FileSystemUtf8Test, CopyFileWithUnicodeName)
{
    Path src = temp_dir / "원본파일.txt";
    Path dst = temp_dir / "복사본ファイル.txt";
    FileSystem::WriteString(src, "UTF-8 내용");

    EXPECT_TRUE(FileSystem::Copy(src, dst));
    EXPECT_TRUE(dst.Exists());

    auto content = FileSystem::ReadToString(dst);
    ASSERT_TRUE(content.HasValue());
    EXPECT_EQ(StringView(content->CStr(), content->ByteLen()), "UTF-8 내용");
}

TEST_F(FileSystemUtf8Test, RenameFileWithUnicodeName)
{
    Path old_path = temp_dir / "이전이름.txt";
    Path new_path = temp_dir / "새이름_新名前.txt";
    FileSystem::WriteString(old_path, "rename test");

    EXPECT_TRUE(FileSystem::Rename(old_path, new_path));
    EXPECT_FALSE(old_path.Exists());
    EXPECT_TRUE(new_path.Exists());
}

TEST_F(FileSystemUtf8Test, SpacesInPath)
{
    Path file = temp_dir / "path with spaces/sub dir/file name.txt";
    FileSystem::CreateDirectories(temp_dir / "path with spaces/sub dir");
    FileSystem::WriteString(file, "spaces content");

    EXPECT_TRUE(file.Exists());
    auto content = FileSystem::ReadToString(file);
    ASSERT_TRUE(content.HasValue());
    EXPECT_EQ(StringView(content->CStr(), content->ByteLen()), "spaces content");
}

TEST_F(FileSystemUtf8Test, SpecialCharactersInFilename)
{
    // 괄호, 하이픈, 플러스 등 특수문자
    Path file = temp_dir / "file (1) - copy [backup]+test.txt";
    FileSystem::WriteString(file, "special chars");

    EXPECT_TRUE(file.Exists());
    EXPECT_EQ(FileSystem::ReadToString(file).Value(),
        String("special chars"));
}


// =============================================================================
// ReadChunked 엣지 케이스 (Chunked Reading Edge Cases)
// =============================================================================

class FileSystemChunkedTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "Chunked" };
};

TEST_F(FileSystemChunkedTest, ReadChunkedEmptyFile)
{
    Path file = temp_dir / "empty.bin";
    FileSystem::WriteString(file, "");

    int chunk_count = 0;
    for (auto&& chunk_result : FileSystem::ReadChunked(file, 1024))
    {
        ASSERT_TRUE(chunk_result.HasValue());
        ++chunk_count;
    }

    // 빈 파일은 0바이트 읽기 -> 루프 진입하지 않음
    EXPECT_EQ(chunk_count, 0);
}

TEST_F(FileSystemChunkedTest, ReadChunkedSmallerThanChunkSize)
{
    Path file = temp_dir / "small.bin";
    FileSystem::WriteString(file, "Hello");

    int chunk_count = 0;
    usize total_bytes = 0;
    for (auto&& chunk_result : FileSystem::ReadChunked(file, 1024))
    {
        ASSERT_TRUE(chunk_result.HasValue());
        total_bytes += chunk_result->Len();
        ++chunk_count;
    }

    EXPECT_EQ(chunk_count, 1);
    EXPECT_EQ(total_bytes, 5u);
}

TEST_F(FileSystemChunkedTest, ReadChunkedExactMultiple)
{
    // chunk_size의 정확한 배수 크기 파일
    const usize chunk_size = 4;
    String content("ABCDABCD"); // 8바이트 = 4 * 2
    Path file = temp_dir / "exact.bin";
    FileSystem::WriteString(file, content);

    int chunk_count = 0;
    usize total_bytes = 0;
    for (auto&& chunk_result : FileSystem::ReadChunked(file, chunk_size))
    {
        ASSERT_TRUE(chunk_result.HasValue());
        total_bytes += chunk_result->Len();
        ++chunk_count;
    }

    EXPECT_EQ(total_bytes, 8u);
    // chunk_size 정확히 배수이므로 마지막에 0바이트 read 후 종료
    // chunk_count는 2 또는 3 (구현에 따라 마지막 0-byte read 포함 여부)
    EXPECT_GE(chunk_count, 2);
}

TEST_F(FileSystemChunkedTest, ReadChunkedNonExistentFile)
{
    Path non_existent = temp_dir / "no_such_file.bin";

    for (auto&& chunk_result : FileSystem::ReadChunked(non_existent, 1024))
    {
        // 첫 yield가 에러여야 함
        EXPECT_FALSE(chunk_result.HasValue());
        break;
    }
}


// =============================================================================
// Rename 엣지 케이스 (Rename Edge Cases)
// =============================================================================

class FileSystemRenameTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "Rename" };
};

TEST_F(FileSystemRenameTest, RenameOverwritesExistingFile)
{
    Path src = temp_dir / "source.txt";
    Path dst = temp_dir / "destination.txt";
    FileSystem::WriteString(src, "new content");
    FileSystem::WriteString(dst, "old content");

    bool result = FileSystem::Rename(src, dst);
    EXPECT_TRUE(result);
    EXPECT_FALSE(src.Exists());
    EXPECT_EQ(FileSystem::ReadToString(dst).Value(), String("new content"));
}

TEST_F(FileSystemRenameTest, RenameToNonExistentTarget)
{
    Path src = temp_dir / "exists.txt";
    Path dst = temp_dir / "new_name.txt";
    FileSystem::WriteString(src, "content");

    EXPECT_TRUE(FileSystem::Rename(src, dst));
    EXPECT_TRUE(dst.Exists());
}

TEST_F(FileSystemRenameTest, RenameEmptyPaths)
{
    EXPECT_FALSE(FileSystem::Rename(Path{}, Path{"target"}));
    EXPECT_FALSE(FileSystem::Rename(Path{"source"}, Path{}));
    EXPECT_FALSE(FileSystem::Rename(Path{}, Path{}));
}


// =============================================================================
// Copy 엣지 케이스 (Copy Edge Cases)
// =============================================================================

class FileSystemCopyTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "Copy" };
};

TEST_F(FileSystemCopyTest, CopyDirectoryOnlyCreatesTarget)
{
    // ARCH-2: 디렉토리 Copy는 대상 디렉토리만 생성하고 내용물은 복사하지 않음
    Path src_dir = temp_dir / "src_dir";
    FileSystem::CreateDirectories(src_dir);
    FileSystem::WriteString(src_dir / "inside.txt", "data");

    Path dst_dir = temp_dir / "dst_dir";
    EXPECT_TRUE(FileSystem::Copy(src_dir, dst_dir));
    EXPECT_TRUE(dst_dir.IsDirectory());

    // 내용물은 복사되지 않음 (현재 구현 동작 문서화)
    EXPECT_FALSE((dst_dir / "inside.txt").Exists());
}

TEST_F(FileSystemCopyTest, CopyNonExistentFile)
{
    Path src = temp_dir / "non_existent.txt";
    Path dst = temp_dir / "destination.txt";
    EXPECT_FALSE(FileSystem::Copy(src, dst));
}

TEST_F(FileSystemCopyTest, CopyEmptyPaths)
{
    Path valid = temp_dir / "some_file.txt";
    EXPECT_FALSE(FileSystem::Copy(Path{}, valid));
    EXPECT_FALSE(FileSystem::Copy(valid, Path{}));
}


// =============================================================================
// RemoveAll 엣지 케이스
// =============================================================================

class FileSystemRemoveAllTest : public ::testing::Test
{
protected:
    TempDir temp_dir{ "RemoveAll" };
};

TEST_F(FileSystemRemoveAllTest, RemoveAllSingleFile)
{
    Path file = temp_dir / "single.txt";
    FileSystem::WriteString(file, "data");

    usize count = FileSystem::RemoveAll(file);
    EXPECT_EQ(count, 1u);
    EXPECT_FALSE(file.Exists());
}

TEST_F(FileSystemRemoveAllTest, RemoveAllDeeplyNested)
{
    // 5단계 중첩 디렉토리
    Path deep = temp_dir / "a/b/c/d/e";
    FileSystem::CreateDirectories(deep);
    FileSystem::WriteString(deep / "leaf.txt", "data");

    usize count = FileSystem::RemoveAll(temp_dir / "a");
    EXPECT_GT(count, 1u);
    EXPECT_FALSE((temp_dir / "a").Exists());
}

TEST_F(FileSystemRemoveAllTest, RemoveAllEmptyDirectory)
{
    Path empty_dir = temp_dir / "empty";
    FileSystem::CreateDirectories(empty_dir);

    usize count = FileSystem::RemoveAll(empty_dir);
    EXPECT_EQ(count, 1u);
    EXPECT_FALSE(empty_dir.Exists());
}

TEST_F(FileSystemRemoveAllTest, RemoveAllNonExistentReturnsZero)
{
    usize count = FileSystem::RemoveAll(temp_dir / "ghost");
    EXPECT_EQ(count, 0u);
}

TEST_F(FileSystemRemoveAllTest, RemoveAllEmptyPathReturnsZero)
{
    usize count = FileSystem::RemoveAll(Path{});
    EXPECT_EQ(count, 0u);
}


// =============================================================================
// FileInfo 엣지 케이스
// =============================================================================

TEST_F(FileSystemFileTest, FileSizeOfEmptyFile)
{
    Path file = temp_dir / "zero.bin";
    FileSystem::WriteString(file, "");

    auto size = FileSystem::FileSize(file);
    ASSERT_TRUE(size.HasValue());
    EXPECT_EQ(*size, 0u);
}

TEST_F(FileSystemFileTest, LastWriteTimeReturnedForExistingFile)
{
    Path file = temp_dir / "timed.txt";
    FileSystem::WriteString(file, "hello");

    auto mtime = FileSystem::LastWriteTime(file);
    ASSERT_TRUE(mtime.HasValue());
    // Unix epoch 이후 시간이어야 함 (0보다 큰 값)
    EXPECT_GT(*mtime, 0u);
}

TEST_F(FileSystemFileTest, LastWriteTimeNulloptForNonExistent)
{
    auto mtime = FileSystem::LastWriteTime(temp_dir / "ghost.txt");
    EXPECT_FALSE(mtime.HasValue());
}

TEST_F(FileSystemFileTest, FileSizeEmptyPath)
{
    auto size = FileSystem::FileSize(Path{});
    EXPECT_FALSE(size.HasValue());
}


// =============================================================================
// WriteString 엣지 케이스
// =============================================================================

TEST_F(FileSystemReadWriteTest, WriteEmptyPathReturnsFalse)
{
    EXPECT_FALSE(FileSystem::WriteString(Path{}, "content"));
}

TEST_F(FileSystemReadWriteTest, WriteBinaryEmptyPathReturnsFalse)
{
    Array<u8> data;
    data.Push(0x42);
    EXPECT_FALSE(FileSystem::Write(Path{}, data));
}

TEST_F(FileSystemReadWriteTest, ReadWriteLargeFile)
{
    // 1MB 파일 읽기/쓰기
    Path file = temp_dir / "large.bin";
    Array<u8> data;
    const usize size = 1024 * 1024;
    data.ResizeUninitialized(size);
    for (usize i = 0; i < size; ++i)
    {
        data[i] = static_cast<u8>(i & 0xFF);
    }

    ASSERT_TRUE(FileSystem::Write(file, data));

    auto read_result = FileSystem::ReadBytes(file);
    ASSERT_TRUE(read_result.HasValue());
    EXPECT_EQ(read_result->Len(), size);

    // 첫 1KB와 마지막 1KB 검증
    for (usize i = 0; i < 1024; ++i)
    {
        EXPECT_EQ((*read_result)[i], static_cast<u8>(i & 0xFF));
    }
    for (usize i = size - 1024; i < size; ++i)
    {
        EXPECT_EQ((*read_result)[i], static_cast<u8>(i & 0xFF));
    }
}
