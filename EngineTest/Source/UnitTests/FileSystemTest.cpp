#include "gtest/gtest.h"

#include <filesystem>

#include "SimpleEngine/Core/Container/Array.h"
#include "SimpleEngine/Core/Types/Path.h"
#include "SimpleEngine/Utility/FileSystem.h"

using namespace se;


namespace
{
// 테스트용 임시 디렉토리 관리자
class TempDir
{
public:
    TempDir(std::string_view name)
        : root_path(std::filesystem::temp_directory_path() / "SimpleEngineTest" / "FileSystemTest" / std::string(name))
    {
        std::filesystem::create_directories(root_path);
    }

    ~TempDir()
    {
        std::error_code ec;
        std::filesystem::remove_all(root_path, ec);
    }

    Path GetPath() const { return Path{ root_path }; }

    // UTF-8 문자열을 올바르게 처리하기 위해 char8_t로 변환
    Path operator/(std::string_view relative) const
    {
        std::filesystem::path relative_path{ reinterpret_cast<const char8_t*>(relative.data()),
                                             reinterpret_cast<const char8_t*>(relative.data() + relative.size()) };
        return Path{ root_path / relative_path };
    }

private:
    std::filesystem::path root_path;
};
}  // namespace


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
    FileSystem::CreateDirectory(temp_dir / "subdir");

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

TEST_F(FileSystemDirTest, CreateDirectorySucceeds)
{
    Path new_dir = temp_dir / "new_directory";
    EXPECT_FALSE(new_dir.Exists());

    bool result = FileSystem::CreateDirectory(new_dir);

    EXPECT_TRUE(result);
    EXPECT_TRUE(new_dir.Exists());
    EXPECT_TRUE(new_dir.IsDirectory());
}

TEST_F(FileSystemDirTest, CreateDirectoryReturnsTrueIfAlreadyExists)
{
    Path existing_dir = temp_dir / "existing_directory";
    FileSystem::CreateDirectory(existing_dir);
    ASSERT_TRUE(existing_dir.Exists());

    bool result = FileSystem::CreateDirectory(existing_dir);
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
    const std::string_view content = "Hello, World!";
    FileSystem::WriteString(file_path, content);

    Optional<usize> size = FileSystem::FileSize(file_path);

    ASSERT_TRUE(size.HasValue());
    EXPECT_EQ(size.Value(), content.size());
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
    const std::string_view content = "Hello, SimpleEngine!\n이것은 UTF-8 테스트입니다.";

    bool write_result = FileSystem::WriteString(file_path, content);
    ASSERT_TRUE(write_result);
    ASSERT_TRUE(file_path.Exists());

    Optional<String> read_result = FileSystem::ReadToString(file_path);
    ASSERT_TRUE(read_result.HasValue());
    EXPECT_EQ(std::string_view(read_result->CStr(), read_result->ByteLen()), content);
}

TEST_F(FileSystemReadWriteTest, WriteAndReadBinaryData)
{
    Path file_path = temp_dir / "binary_file.bin";

    // 바이너리 데이터 생성
    Array<uint8> original_data;
    for (int i = 0; i < 256; ++i)
    {
        original_data.Push(static_cast<uint8>(i));
    }

    bool write_result = FileSystem::Write(file_path, original_data);
    ASSERT_TRUE(write_result);

    Optional<Array<uint8>> read_result = FileSystem::Read(file_path);
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
    Optional<String> result = FileSystem::ReadToString(non_existent);

    EXPECT_FALSE(result.HasValue());
}

TEST_F(FileSystemReadWriteTest, ReadReturnsNulloptForNonExistent)
{
    Path non_existent = temp_dir / "non_existent.bin";
    Optional<Array<uint8>> result = FileSystem::Read(non_existent);

    EXPECT_FALSE(result.HasValue());
}

TEST_F(FileSystemReadWriteTest, WriteStringOverwritesExistingContent)
{
    Path file_path = temp_dir / "overwrite.txt";

    FileSystem::WriteString(file_path, "original content");
    FileSystem::WriteString(file_path, "new content");

    auto result = FileSystem::ReadToString(file_path);
    ASSERT_TRUE(result.HasValue());
    EXPECT_EQ(std::string_view(result->CStr(), result->ByteLen()), "new content");
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
        FileSystem::CreateDirectory(temp_dir / "subdir1");
        FileSystem::CreateDirectory(temp_dir / "subdir2");
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
    FileSystem::CreateDirectory(empty_dir);

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
    const std::string_view content = "한글 내용입니다.";

    bool write_result = FileSystem::WriteString(korean_path, content);
    ASSERT_TRUE(write_result);

    Optional<String> read_result = FileSystem::ReadToString(korean_path);
    ASSERT_TRUE(read_result.HasValue());
    EXPECT_EQ(std::string_view(read_result->CStr(), read_result->ByteLen()), content);
}

TEST_F(FileSystemUtf8Test, CreateDirectoryWithUnicodeName)
{
    Path unicode_dir = temp_dir / "日本語フォルダ";

    bool result = FileSystem::CreateDirectory(unicode_dir);

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
