#include "gtest/gtest.h"
#include "SimpleEngine/Core/Types/Path.h"

using namespace se;


// =============================================================================
// 기본 기능 테스트 (Basic Functionality)
// =============================================================================

TEST(PathTest, Constructors)
{
    Path p1;
    EXPECT_TRUE(p1.IsEmpty());

    Path p2("Engine/Source/Core.cpp");
    EXPECT_FALSE(p2.IsEmpty());

    String s = "Engine/Source/Main.cpp";
    Path p3(s);
    EXPECT_EQ(p3.ToString(), s);
}

TEST(PathTest, ConstructorNullptr)
{
    const char* null_ptr = nullptr;
    Path p(null_ptr);
    EXPECT_TRUE(p.IsEmpty());
}

TEST(PathTest, Modifiers)
{
    Path p("Engine");
    p.Append(Path("Source"));
    // Note: Comparison uses Path's internal logic or ToString()
    EXPECT_EQ(p.ToString(), "Engine/Source");

    p /= Path("Core");
    EXPECT_EQ(p.ToString(), "Engine/Source/Core");

    p.Concat(".cpp");
    EXPECT_EQ(p.ToString(), "Engine/Source/Core.cpp");

    p += ".tmp";
    EXPECT_EQ(p.ToString(), "Engine/Source/Core.cpp.tmp");

    Path p2("A/./B/../C"); // 생성자에서 정규화 됨
    EXPECT_EQ(p2.ToString(), "A/C");

    Path p3("dir/old.txt");
    p3.SetFileName("new.png");
    EXPECT_EQ(p3.ToString(), "dir/new.png");

    Path p4("file");
    p4.SetExtension("txt");
    EXPECT_EQ(p4.ToString(), "file.txt");

    p4.SetExtension(".jpg");
    EXPECT_EQ(p4.ToString(), "file.jpg");
}

TEST(PathTest, Producers)
{
    Path base("Engine");
    Path full = base / Path("Source/Main.cpp");
    EXPECT_EQ(full.ToString(), "Engine/Source/Main.cpp");

    Path p1("dir/file.txt");
    EXPECT_EQ(p1.WithFileName("new.png").ToString(), "dir/new.png");
    EXPECT_EQ(p1.WithExtension(".log").ToString(), "dir/file.log");

    Path p2("A/./B/../C");
    EXPECT_EQ(p2.ToString(), "A/C");

    Path p3("/A/B/C");
    auto rel = p3.RelativeTo(Path("/A"));
    ASSERT_TRUE(rel.HasValue());
    EXPECT_EQ(rel.Value().ToString(), "B/C");
}

TEST(PathTest, Components)
{
    Path p("C:/Project/Game/Source/Main.cpp");

    // Parent() -> Optional<Path>
    auto parent = p.Parent();
    ASSERT_TRUE(parent.HasValue());
    EXPECT_EQ(parent.Value().ToString(), "C:/Project/Game/Source");

    // FileName() -> Optional<String>
    auto filename = p.FileName();
    ASSERT_TRUE(filename.HasValue());
    EXPECT_EQ(filename.Value(), "Main.cpp");

    // FileStem() -> Optional<String>
    auto stem = p.FileStem();
    ASSERT_TRUE(stem.HasValue());
    EXPECT_EQ(stem.Value(), "Main");

    // Extension() -> Optional<String>
    auto ext = p.Extension();
    ASSERT_TRUE(ext.HasValue());
    EXPECT_EQ(ext.Value(), ".cpp");

    // Empty cases
    Path root("/");
    // Some FS might return nullopt for root parent
    EXPECT_FALSE(root.Parent().HasValue());
    EXPECT_FALSE(root.Extension().HasValue());
}

TEST(PathTest, Queries)
{
#if defined(SE_PLATFORM_WINDOWS)
    Path p("C:/A/B/C.txt");
#else
    Path p("/A/B/C.txt");
#endif

    EXPECT_FALSE(p.IsEmpty());
    EXPECT_TRUE(p.IsAbsolute());
    EXPECT_FALSE(p.IsRelative());

#if defined(SE_PLATFORM_WINDOWS)
    EXPECT_TRUE(p.IsSubPathOf(Path("C:/A")));
    EXPECT_TRUE(p.IsSubPathOf(Path("C:/A/B")));
    EXPECT_FALSE(p.IsSubPathOf(Path("C:/D")));
#else
    EXPECT_TRUE(p.IsSubPathOf(Path("/A")));
    EXPECT_TRUE(p.IsSubPathOf(Path("/A/B")));
    EXPECT_FALSE(p.IsSubPathOf(Path("/D")));
#endif
}

TEST(PathTest, Equality)
{
    Path p("A/B");

    EXPECT_TRUE(p == "A/B");
    EXPECT_TRUE(p == String("A/B"));
    EXPECT_TRUE(p == Path("A/B"));

    EXPECT_FALSE(p == "A/C");

    EXPECT_EQ(p <=> Path("A/A"), std::strong_ordering::greater);
}


// =============================================================================
// 경로 정규화 엣지 케이스 (Normalization Edge Cases)
// =============================================================================

TEST(PathNormalizationTest, BackslashConversion)
{
    // 백슬래시를 포워드 슬래시로 변환
    Path p("A\\B\\C\\file.txt");
    EXPECT_EQ(p.ToString(), "A/B/C/file.txt");

    // 혼용된 구분자
    Path p2("A/B\\C/D\\file.txt");
    EXPECT_EQ(p2.ToString(), "A/B/C/D/file.txt");
}

TEST(PathNormalizationTest, MultipleConsecutiveSlashes)
{
    // 이중 슬래시 (비-UNC)
    Path p("A//B///C////file.txt");
    EXPECT_EQ(p.ToString(), "A/B/C/file.txt");

    // 중간의 이중 슬래시
    Path p2("dir//file.txt");
    EXPECT_EQ(p2.ToString(), "dir/file.txt");
}

TEST(PathNormalizationTest, TrailingSlashRemoval)
{
    Path p("dir/subdir/");
    EXPECT_EQ(p.ToString(), "dir/subdir");

    Path p2("dir/subdir///");
    EXPECT_EQ(p2.ToString(), "dir/subdir");
}

TEST(PathNormalizationTest, DotSegments)
{
    // 현재 디렉토리 "." 제거
    Path p1("A/./B/./C");
    EXPECT_EQ(p1.ToString(), "A/B/C");

    // 단독 "."은 "."으로 유지되어야 함 (상대 경로의 현재 디렉토리)
    Path p2(".");
    EXPECT_EQ(p2.ToString(), ".");

    // "./"으로 시작하는 경로
    Path p3("./A/B");
    EXPECT_EQ(p3.ToString(), "A/B");
}

TEST(PathNormalizationTest, DotDotSegments)
{
    // 기본 ".." 해소
    Path p1("A/B/../C");
    EXPECT_EQ(p1.ToString(), "A/C");

    // 연속 ".."
    Path p2("A/B/C/../../D");
    EXPECT_EQ(p2.ToString(), "A/D");

    // 모든 세그먼트를 소진하는 ".." (상대 경로)
    Path p3("A/B/../../C");
    EXPECT_EQ(p3.ToString(), "C");

    // 루트 위로 탈출 시도 (절대 경로에서 무시됨)
    Path p4("/A/../../../B");
    EXPECT_EQ(p4.ToString(), "/B");

    // 상대 경로에서 루트 위로의 ".."는 보존
    Path p5("A/../../B");
    EXPECT_EQ(p5.ToString(), "../B");

    // 상대 경로에서 연속 ".." 보존
    Path p6("../../A/B");
    EXPECT_EQ(p6.ToString(), "../../A/B");
}

TEST(PathNormalizationTest, ComplexDotDotChain)
{
    // ".." 뒤에 ".."이 있어 이전 ".."을 pop하지 않는지 확인
    Path p("../A/../..");
    EXPECT_EQ(p.ToString(), "../..");

    // 정규화가 모든 세그먼트를 제거하면 "."
    Path p2("A/..");
    EXPECT_EQ(p2.ToString(), ".");

    Path p3("A/B/../..");
    EXPECT_EQ(p3.ToString(), ".");
}

TEST(PathNormalizationTest, OnlyDotsInFilename)
{
    // "..." (3개 점)은 일반 파일명으로 취급
    Path p("dir/.../file");
    EXPECT_EQ(p.ToString(), "dir/.../file");
}

TEST(PathNormalizationTest, EmptyInput)
{
    Path p1("");
    EXPECT_TRUE(p1.IsEmpty());

    Path p2(String{});
    EXPECT_TRUE(p2.IsEmpty());

    Path p3(StringView{});
    EXPECT_TRUE(p3.IsEmpty());
}


// =============================================================================
// Windows 드라이브 경로 (Windows Drive Paths)
// =============================================================================

TEST(PathWindowsTest, DriveAbsolutePath)
{
    Path p("C:/Users/Game/file.txt");
    EXPECT_TRUE(p.IsAbsolute());
    EXPECT_EQ(p.ToString(), "C:/Users/Game/file.txt");
}

TEST(PathWindowsTest, DriveRelativePath)
{
    // "C:foo"는 드라이브-상대 경로 (드라이브 C의 현재 디렉토리 기준)
    // IsAbsolute()는 false여야 함 (C:/ 형태가 아님)
    Path p("C:foo");
    EXPECT_FALSE(p.IsAbsolute());
    EXPECT_TRUE(p.IsRelative());
}

TEST(PathWindowsTest, DriveWithBackslash)
{
    Path p("C:\\Users\\Game\\file.txt");
    EXPECT_TRUE(p.IsAbsolute());
    EXPECT_EQ(p.ToString(), "C:/Users/Game/file.txt");
}

TEST(PathWindowsTest, DriveRootParent)
{
    Path p("C:/file.txt");
    auto parent = p.Parent();
    ASSERT_TRUE(parent.HasValue());
    EXPECT_EQ(parent->ToString(), "C:/");

    // 드라이브 루트 자체의 Parent는 NullOpt
    EXPECT_FALSE(parent->Parent().HasValue());
}

TEST(PathWindowsTest, DriveRootComponents)
{
    Path root("C:/");
    EXPECT_TRUE(root.IsAbsolute());
    EXPECT_FALSE(root.Parent().HasValue());
    EXPECT_FALSE(root.FileName().HasValue());
    EXPECT_FALSE(root.Extension().HasValue());
}

TEST(PathWindowsTest, DotDotBeyondDriveRoot)
{
    // C:/ 위로 ".." 시도 시 무시됨
    Path p("C:/A/../../B");
    EXPECT_EQ(p.ToString(), "C:/B");
}

TEST(PathWindowsTest, DifferentDriveLetterRelativeTo)
{
    // 다른 드라이브 간 RelativeTo는 NullOpt
    Path p1("C:/A/B");
    Path p2("D:/A/B");
    EXPECT_FALSE(p1.RelativeTo(p2).HasValue());
}

TEST(PathWindowsTest, CaseSensitiveComparison)
{
    // 의도적으로 case-sensitive: 크로스 플랫폼 예측 가능성
    Path p1("C:/Users/Game");
    Path p2("C:/users/game");
    EXPECT_NE(p1, p2);
}

TEST(PathWindowsTest, LowercaseDriveLetter)
{
    Path p("c:/Users/file.txt");
    EXPECT_TRUE(p.IsAbsolute());
    EXPECT_EQ(p.ToString(), "c:/Users/file.txt");
}


// =============================================================================
// UNC 경로 (UNC Paths)
// =============================================================================

TEST(PathUNCTest, BasicUNCPath)
{
    Path p("//server/share/dir/file.txt");
    EXPECT_TRUE(p.IsAbsolute());
    EXPECT_EQ(p.ToString(), "//server/share/dir/file.txt");
}

TEST(PathUNCTest, UNCWithBackslash)
{
    Path p("\\\\server\\share\\dir\\file.txt");
    EXPECT_TRUE(p.IsAbsolute());
    EXPECT_EQ(p.ToString(), "//server/share/dir/file.txt");
}

TEST(PathUNCTest, UNCParent)
{
    Path p("//server/share/dir/file.txt");
    auto parent = p.Parent();
    ASSERT_TRUE(parent.HasValue());
    EXPECT_EQ(parent->ToString(), "//server/share/dir");

    // UNC 루트까지 올라가기
    Path p2("//server/share/file.txt");
    auto parent2 = p2.Parent();
    ASSERT_TRUE(parent2.HasValue());
    EXPECT_EQ(parent2->ToString(), "//server/share");

    // UNC 루트의 Parent는 NullOpt
    EXPECT_FALSE(parent2->Parent().HasValue());
}

TEST(PathUNCTest, UNCDotDotBeyondRoot)
{
    // UNC 루트 위로 ".." 시도
    Path p("//server/share/../../../escape");
    EXPECT_EQ(p.ToString(), "//server/share/escape");
}

TEST(PathUNCTest, UNCFileName)
{
    Path p("//server/share");
    // UNC 루트 자체는 파일명 없음
    EXPECT_FALSE(p.FileName().HasValue());
}


// =============================================================================
// Unix 절대 경로 (Unix Absolute Paths)
// =============================================================================

TEST(PathUnixTest, RootPath)
{
    Path p("/");
    EXPECT_TRUE(p.IsAbsolute());
    EXPECT_FALSE(p.Parent().HasValue());
    EXPECT_FALSE(p.FileName().HasValue());
}

TEST(PathUnixTest, SingleFileInRoot)
{
    Path p("/file.txt");
    EXPECT_TRUE(p.IsAbsolute());

    auto parent = p.Parent();
    ASSERT_TRUE(parent.HasValue());
    EXPECT_EQ(parent->ToString(), "/");

    auto filename = p.FileName();
    ASSERT_TRUE(filename.HasValue());
    EXPECT_EQ(*filename, "file.txt");
}


// =============================================================================
// 숨김 파일 및 특수 확장자 (Hidden Files & Special Extensions)
// =============================================================================

TEST(PathHiddenFileTest, DotFileNoExtension)
{
    // ".gitignore"는 숨김파일 — 전체가 stem, 확장자 없음
    Path p("dir/.gitignore");
    auto stem = p.FileStem();
    auto ext = p.Extension();

    ASSERT_TRUE(stem.HasValue());
    EXPECT_EQ(*stem, ".gitignore");
    EXPECT_FALSE(ext.HasValue());
}

TEST(PathHiddenFileTest, DotFileWithExtension)
{
    // ".bashrc.bak"은 stem=".bashrc", ext=".bak"
    Path p("dir/.bashrc.bak");
    auto stem = p.FileStem();
    auto ext = p.Extension();

    ASSERT_TRUE(stem.HasValue());
    EXPECT_EQ(*stem, ".bashrc");
    ASSERT_TRUE(ext.HasValue());
    EXPECT_EQ(*ext, ".bak");
}

TEST(PathHiddenFileTest, MultipleExtensions)
{
    Path p("archive.tar.gz");
    auto ext = p.Extension();
    auto stem = p.FileStem();

    ASSERT_TRUE(ext.HasValue());
    EXPECT_EQ(*ext, ".gz");
    ASSERT_TRUE(stem.HasValue());
    EXPECT_EQ(*stem, "archive.tar");
}

TEST(PathHiddenFileTest, FileNameWithOnlyDot)
{
    Path p("dir/.");
    // "." 세그먼트는 정규화에서 제거됨
    EXPECT_EQ(p.ToString(), "dir");
}


// =============================================================================
// SetExtension 정규화 검증 (SetExtension Normalization)
// =============================================================================

TEST(PathSetExtensionTest, ExtensionWithPathSeparator)
{
    // SetExtension에 경로 구분자가 포함된 경우
    // 현재 구현에서는 재정규화하지 않으므로 비정규화 상태가 될 수 있음
    Path p("dir/file.txt");
    p.SetExtension(".jpg/../../../etc/passwd");

    // BUG: SetExtension은 NormalizePath를 호출하지 않아 path traversal이 가능함.
    // 이 테스트는 현재 동작을 문서화함. 수정 후 아래 주석의 기대값으로 변경해야 함.
    // EXPECT_EQ(p.ToString(), "dir/file.jpg/../../../etc/passwd"); // 현재 동작 (비정규화)
    // 수정 후 기대값 예시: NormalizePath 적용 시 "../../etc/passwd" 또는 유사 결과
}

TEST(PathSetExtensionTest, RemoveExtension)
{
    Path p("dir/file.txt");
    p.SetExtension("");

    EXPECT_EQ(p.ToString(), "dir/file");
    EXPECT_FALSE(p.Extension().HasValue());
}

TEST(PathSetExtensionTest, SetExtensionOnFileWithoutExtension)
{
    Path p("dir/README");
    p.SetExtension(".md");

    EXPECT_EQ(p.ToString(), "dir/README.md");
}

TEST(PathSetExtensionTest, SetExtensionOnHiddenFile)
{
    // 숨김파일(".gitignore")에는 확장자가 없으므로 새 확장자가 추가됨
    Path p("dir/.gitignore");
    p.SetExtension(".bak");

    EXPECT_EQ(p.ToString(), "dir/.gitignore.bak");
}


// =============================================================================
// RelativeTo 엣지 케이스 (RelativeTo Edge Cases)
// =============================================================================

TEST(PathRelativeToTest, AbsoluteRelativeMismatch)
{
    Path abs("/A/B");
    Path rel("A/B");
    EXPECT_FALSE(abs.RelativeTo(rel).HasValue());
    EXPECT_FALSE(rel.RelativeTo(abs).HasValue());
}

TEST(PathRelativeToTest, SamePath)
{
    Path p("/A/B/C");
    auto rel = p.RelativeTo(p);
    ASSERT_TRUE(rel.HasValue());
    EXPECT_EQ(rel->ToString(), ".");
}

TEST(PathRelativeToTest, ChildPath)
{
    Path child("/A/B/C/D");
    Path base("/A/B");
    auto rel = child.RelativeTo(base);
    ASSERT_TRUE(rel.HasValue());
    EXPECT_EQ(rel->ToString(), "C/D");
}

TEST(PathRelativeToTest, ParentPath)
{
    Path parent("/A");
    Path child("/A/B/C");
    auto rel = parent.RelativeTo(child);
    ASSERT_TRUE(rel.HasValue());
    EXPECT_EQ(rel->ToString(), "../..");
}

TEST(PathRelativeToTest, SiblingPath)
{
    Path p1("/A/B/C");
    Path p2("/A/B/D");
    auto rel = p1.RelativeTo(p2);
    ASSERT_TRUE(rel.HasValue());
    EXPECT_EQ(rel->ToString(), "../C");
}

TEST(PathRelativeToTest, NoCommonPrefix)
{
    Path p1("/A/B");
    Path p2("/C/D");
    EXPECT_FALSE(p1.RelativeTo(p2).HasValue());
}

TEST(PathRelativeToTest, RelativeToRoot)
{
    Path p("/A/B/C");
    Path root("/");
    auto rel = p.RelativeTo(root);
    ASSERT_TRUE(rel.HasValue());
    EXPECT_EQ(rel->ToString(), "A/B/C");
}

TEST(PathRelativeToTest, BothEmpty)
{
    Path p1;
    Path p2;
    // 빈 경로는 상대 경로로 취급, common=0 양쪽 모두 비어있음 → "."
    auto rel = p1.RelativeTo(p2);
    ASSERT_TRUE(rel.HasValue());
    EXPECT_EQ(rel->ToString(), ".");
}


// =============================================================================
// Append 엣지 케이스 (Append Edge Cases)
// =============================================================================

TEST(PathAppendTest, AppendAbsoluteReplacesPath)
{
    // std::filesystem::path 동작과 동일: RHS가 절대 경로이면 대체
    Path p("A/B");
    p.Append(Path("/C/D"));
    EXPECT_EQ(p.ToString(), "/C/D");
}

TEST(PathAppendTest, AppendWindowsAbsoluteReplacesPath)
{
    Path p("some/relative/path");
    p.Append(Path("C:/Absolute/Path"));
    EXPECT_EQ(p.ToString(), "C:/Absolute/Path");
}

TEST(PathAppendTest, AppendEmpty)
{
    Path p("A/B");
    p.Append(Path{});
    EXPECT_EQ(p.ToString(), "A/B");
}

TEST(PathAppendTest, AppendToEmpty)
{
    Path p;
    p.Append(Path("A/B"));
    EXPECT_EQ(p.ToString(), "A/B");
}

TEST(PathAppendTest, AppendWithDotDot)
{
    Path p("A/B/C");
    p.Append(Path("../D"));
    EXPECT_EQ(p.ToString(), "A/B/D");
}


// =============================================================================
// UTF-8 / 유니코드 경로 (Unicode Paths)
// =============================================================================

TEST(PathUnicodeTest, CJKCharacters)
{
    // 한국어, 일본어, 중국어 경로
    Path korean("프로젝트/소스/메인.cpp");
    EXPECT_EQ(korean.FileName().Value(), "메인.cpp");
    EXPECT_EQ(korean.FileStem().Value(), "메인");
    EXPECT_EQ(korean.Extension().Value(), ".cpp");

    Path japanese("プロジェクト/ソース/メイン.cpp");
    EXPECT_EQ(japanese.FileName().Value(), "メイン.cpp");

    Path chinese("项目/源代码/主文件.cpp");
    EXPECT_EQ(chinese.FileName().Value(), "主文件.cpp");
}

TEST(PathUnicodeTest, EmojiInPath)
{
    Path p("🎮/📁/🎵.mp3");
    EXPECT_EQ(p.FileName().Value(), "🎵.mp3");
    EXPECT_EQ(p.FileStem().Value(), "🎵");
    EXPECT_EQ(p.Extension().Value(), ".mp3");
}

TEST(PathUnicodeTest, ArabicRTLPath)
{
    // 아랍어 RTL 문자 경로 (바이트 비교이므로 방향 무관)
    Path p("العربية/ملف.txt");
    EXPECT_EQ(p.FileName().Value(), "ملف.txt");
}

TEST(PathUnicodeTest, MixedScripts)
{
    // ASCII + Unicode 혼합
    Path p("Assets/텍스처/player_スプライト.png");
    EXPECT_EQ(p.FileName().Value(), "player_スプライト.png");
    EXPECT_EQ(p.Extension().Value(), ".png");
}

TEST(PathUnicodeTest, SpacesInPath)
{
    Path p("My Documents/Game Project/Main Scene.unity");
    EXPECT_EQ(p.FileName().Value(), "Main Scene.unity");
    EXPECT_EQ(p.Parent().Value().ToString(), "My Documents/Game Project");
}

TEST(PathUnicodeTest, SpecialCharactersInFilename)
{
    // 파일명에 괄호, 하이픈, 공백
    Path p("dir/player sprite (1) - copy.png");
    EXPECT_EQ(p.FileStem().Value(), "player sprite (1) - copy");
    EXPECT_EQ(p.Extension().Value(), ".png");
}


// =============================================================================
// IsSubPathOf 엣지 케이스 (Containment Check Edge Cases)
// =============================================================================

TEST(PathSubPathTest, SamePathIsSubPath)
{
    Path p("/A/B");
    EXPECT_TRUE(p.IsSubPathOf(p));
}

TEST(PathSubPathTest, DotDotEscapeNotSubPath)
{
    // IsSubPathOf는 ".."로 시작하는 상대 경로를 거부해야 함
    Path p("/A");
    Path base("/A/B");
    EXPECT_FALSE(p.IsSubPathOf(base));
}

TEST(PathSubPathTest, PrefixMatchDoesNotFalsePositive)
{
    // "/A/BC"는 "/A/B"의 하위가 아님 (세그먼트 경계 체크)
    Path p("/A/BC/file.txt");
    Path base("/A/B");
    EXPECT_FALSE(p.IsSubPathOf(base));
}


// =============================================================================
// Swap 및 이동 의미론 (Swap & Move Semantics)
// =============================================================================

TEST(PathMoveTest, MoveConstruction)
{
    Path original("A/B/C");
    Path moved(std::move(original));
    EXPECT_EQ(moved.ToString(), "A/B/C");
}

TEST(PathMoveTest, SwapPaths)
{
    Path a("X/Y");
    Path b("A/B");
    swap(a, b);
    EXPECT_EQ(a.ToString(), "A/B");
    EXPECT_EQ(b.ToString(), "X/Y");
}


// =============================================================================
// Hash 및 Formatter (Hash & Formatting)
// =============================================================================

TEST(PathHashTest, EqualPathsHaveSameHash)
{
    Path p1("A/B/C");
    Path p2("A/B/C");
    EXPECT_EQ(std::hash<Path>{}(p1), std::hash<Path>{}(p2));
}

TEST(PathHashTest, NormalizedPathsHaveSameHash)
{
    Path p1("A/B/C");
    Path p2("A/./B/../B/C");
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(std::hash<Path>{}(p1), std::hash<Path>{}(p2));
}
