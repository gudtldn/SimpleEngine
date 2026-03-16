#include "gtest/gtest.h"

#include "SimpleEngine/Core/Container/StringView.h"
#include "SimpleEngine/Core/Types/VPath.h"

using namespace se;


// =============================================================================
// Construction & Basic Properties
// =============================================================================

TEST(VPathTest, DefaultConstructorCreatesInvalidPath)
{
    VPath p;
    EXPECT_FALSE(p.IsValid());
    EXPECT_FALSE(static_cast<bool>(p));
}

TEST(VPathTest, ConstructFromCString)
{
    VPath p("Assets://Textures/Player.png");
    EXPECT_TRUE(p.IsValid());
    EXPECT_TRUE(p.HasScheme());
}

TEST(VPathTest, ConstructFromString)
{
    String s = "Assets://Textures/Player.png";
    VPath p(s);
    EXPECT_EQ(p.ToString(), s);
}

TEST(VPathTest, ConstructFromStringView)
{
    StringView sv = "Assets://Textures/Player.png";
    VPath p(sv);
    EXPECT_TRUE(p.IsValid());
}


// =============================================================================
// Scheme Parsing
// =============================================================================

TEST(VPathTest, GetSchemeReturnsCorrectScheme)
{
    VPath p("Assets://Textures/Player.png");
    EXPECT_EQ(p.GetScheme(), "Assets");
}

TEST(VPathTest, HasSchemeReturnsFalseForRelativePath)
{
    VPath p("Textures/Player.png");
    EXPECT_FALSE(p.HasScheme());
    EXPECT_TRUE(p.GetScheme().IsEmpty());
}

TEST(VPathTest, GetSchemeHandlesEmptyScheme)
{
    VPath p("://path/file.txt");
    EXPECT_EQ(p.GetScheme(), "");
}

TEST(VPathTest, GetSchemeHandlesMultipleColons)
{
    VPath p("Scheme://path:with:colons/file.txt");
    EXPECT_EQ(p.GetScheme(), "Scheme");
}


// =============================================================================
// Path Part Extraction
// =============================================================================

TEST(VPathTest, GetPathPartReturnsPathAfterScheme)
{
    VPath p("Assets://Textures/Player.png");
    EXPECT_EQ(p.GetPathPart(), "Textures/Player.png");
}

TEST(VPathTest, GetPathPartReturnsFullPathIfNoScheme)
{
    VPath p("Textures/Player.png");
    EXPECT_EQ(p.GetPathPart(), "Textures/Player.png");
}

TEST(VPathTest, GetPathPartHandlesRootPath)
{
    VPath p("Assets:///file.txt");
    EXPECT_EQ(p.GetPathPart(), "/file.txt");
}


// =============================================================================
// Filename & Extension
// =============================================================================

TEST(VPathTest, GetFilenameReturnsFilename)
{
    VPath p("Assets://Textures/Player.png");
    EXPECT_EQ(p.GetFilename(), "Player.png");
}

TEST(VPathTest, GetFilenameReturnsEmptyForDirectory)
{
    VPath p("Assets://Textures/");
    EXPECT_TRUE(p.GetFilename().IsEmpty());
}

TEST(VPathTest, GetExtensionReturnsExtension)
{
    VPath p("Assets://Textures/Player.png");
    EXPECT_EQ(p.GetExtension(), ".png");
}

TEST(VPathTest, GetExtensionReturnsEmptyForNoExtension)
{
    VPath p("Assets://Textures/README");
    EXPECT_TRUE(p.GetExtension().IsEmpty());
}

TEST(VPathTest, GetStemReturnsStem)
{
    VPath p("Assets://Textures/Player.png");
    EXPECT_EQ(p.GetStem(), "Player");
}

TEST(VPathTest, GetStemReturnsFilenameIfNoExtension)
{
    VPath p("Assets://Textures/README");
    EXPECT_EQ(p.GetStem(), "README");
}

TEST(VPathTest, GetExtensionHandlesMultipleDots)
{
    VPath p("Assets://Archives/data.tar.gz");
    EXPECT_EQ(p.GetExtension(), ".gz");
    EXPECT_EQ(p.GetStem(), "data.tar");
}


// =============================================================================
// Parent Path
// =============================================================================

TEST(VPathTest, GetParentPathReturnsParent)
{
    VPath p("Assets://Textures/Player.png");
    VPath parent = p.GetParentPath();
    EXPECT_EQ(parent.ToString(), "Assets://Textures");
}

TEST(VPathTest, GetParentPathHandlesNestedPaths)
{
    VPath p("Assets://A/B/C/file.txt");
    EXPECT_EQ(p.GetParentPath().ToString(), "Assets://A/B/C");
    EXPECT_EQ(p.GetParentPath().GetParentPath().ToString(), "Assets://A/B");
}

TEST(VPathTest, GetParentPathOfRootReturnsSchemeOnly)
{
    VPath p("Assets://file.txt");
    VPath parent = p.GetParentPath();
    EXPECT_EQ(parent.ToString(), "Assets://");
}


// =============================================================================
// Path Concatenation
// =============================================================================

TEST(VPathTest, SlashOperatorAppendsRelativePath)
{
    VPath base("Assets://Textures");
    VPath result = base / "Player.png";
    EXPECT_EQ(result.ToString(), "Assets://Textures/Player.png");
}

TEST(VPathTest, SlashOperatorHandlesTrailingSlash)
{
    VPath base("Assets://Textures/");
    VPath result = base / "Player.png";
    EXPECT_EQ(result.ToString(), "Assets://Textures/Player.png");
}

TEST(VPathTest, SlashOperatorHandlesLeadingSlash)
{
    VPath base("Assets://Textures");
    VPath result = base / "/Player.png";
    EXPECT_EQ(result.ToString(), "Assets://Textures/Player.png");
}


// =============================================================================
// Path Normalization
// =============================================================================

TEST(VPathTest, NormalizesBackslashesToForwardSlashes)
{
    VPath p("Assets://Textures\\Player\\sprite.png");
    EXPECT_EQ(p.ToString(), "Assets://Textures/Player/sprite.png");
}

TEST(VPathTest, PreservesSchemeWithBackslashes)
{
    VPath p("Assets:\\\\Textures\\file.png");
    // 백슬래시는 정규화되어야 함
    EXPECT_EQ(p.GetScheme(), "Assets");
}


// =============================================================================
// Comparison & Equality
// =============================================================================

TEST(VPathTest, EqualityComparison)
{
    VPath p1("Assets://Textures/Player.png");
    VPath p2("Assets://Textures/Player.png");
    VPath p3("Assets://Textures/Enemy.png");

    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 == p3);
}

TEST(VPathTest, SpaceshipOperator)
{
    VPath a("Assets://A.txt");
    VPath b("Assets://B.txt");

    EXPECT_TRUE(a < b);
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(a <= a);
    EXPECT_TRUE(a >= a);
}


// =============================================================================
// Conversions
// =============================================================================

TEST(VPathTest, ToStringReturnsFullPath)
{
    VPath p("Assets://Textures/Player.png");
    EXPECT_EQ(p.ToString(), "Assets://Textures/Player.png");
}

TEST(VPathTest, ToStringNameCreatesStringName)
{
    VPath p("Assets://Textures/Player.png");
    StringName sn = p.ToStringName();
    EXPECT_EQ(sn.ToString(), "Assets://Textures/Player.png");
}


// =============================================================================
// Edge Cases
// =============================================================================

TEST(VPathTest, EmptyPathIsInvalid)
{
    VPath p("");
    EXPECT_FALSE(p.IsValid());
}

TEST(VPathTest, SchemeOnlyPathIsValid)
{
    VPath p("Assets://");
    EXPECT_TRUE(p.IsValid());
    EXPECT_TRUE(p.HasScheme());
    EXPECT_EQ(p.GetScheme(), "Assets");
    EXPECT_TRUE(p.GetPathPart().IsEmpty());
}

TEST(VPathTest, VeryLongPathHandledCorrectly)
{
    String long_path = "Assets://";
    for (int i = 0; i < 100; ++i)
    {
        long_path += "very_long_directory_name/";
    }
    long_path += "file.txt";

    VPath p(long_path);
    EXPECT_TRUE(p.IsValid());
    EXPECT_EQ(p.GetFilename(), "file.txt");
}

TEST(VPathTest, SpecialCharactersInPath)
{
    VPath p("Assets://Textures/player sprite (1).png");
    EXPECT_EQ(p.GetFilename(), "player sprite (1).png");
}

TEST(VPathTest, UnicodeInPath)
{
    VPath p("Assets://텍스처/플레이어.png");
    EXPECT_TRUE(p.IsValid());
    EXPECT_EQ(p.GetFilename(), "플레이어.png");
}


// =============================================================================
// operator/ 메타데이터 보존 (BUG-1: Scheme Metadata After Concatenation)
// =============================================================================

TEST(VPathSlashOpTest, PreservesSchemeMetadataAfterAppend)
{
    // BUG: operator/가 scheme_len과 path_offset을 복사하지 않음
    VPath base("Assets://Textures");
    VPath result = base / "Player.png";

    EXPECT_EQ(result.ToString(), "Assets://Textures/Player.png");

    // 이 검증들이 BUG-1이 존재하면 실패함
    EXPECT_TRUE(result.HasScheme());
    EXPECT_EQ(result.GetScheme(), "Assets");
    EXPECT_EQ(result.GetPathPart(), "Textures/Player.png");
}

TEST(VPathSlashOpTest, PreservesSchemeInChainedAppend)
{
    VPath base("Config://");
    VPath result = base / "settings" / "graphics.toml";

    // 2회 체인 후에도 스킴이 보존되어야 함
    EXPECT_TRUE(result.HasScheme());
    EXPECT_EQ(result.GetScheme(), "Config");
    EXPECT_EQ(result.GetFilename(), "graphics.toml");
}

TEST(VPathSlashOpTest, GetParentAfterAppendPreservesScheme)
{
    VPath base("Assets://Textures");
    VPath result = base / "Player.png";
    VPath parent = result.GetParentPath();

    // 부모 경로도 올바른 스킴을 가져야 함
    EXPECT_TRUE(parent.HasScheme());
    EXPECT_EQ(parent.GetScheme(), "Assets");
    EXPECT_EQ(parent.ToString(), "Assets://Textures");
}


// =============================================================================
// operator/ 이중 슬래시 (BUG-3: Double Slash)
// =============================================================================

TEST(VPathSlashOpTest, BothTrailingAndLeadingSlash)
{
    // base 끝이 '/', relative 시작이 '/' → 이중 슬래시 방지
    VPath base("Assets://Textures/");
    VPath result = base / "/Player.png";

    // 이중 슬래시 "Assets://Textures//Player.png"가 되면 안 됨
    EXPECT_EQ(result.ToString(), "Assets://Textures/Player.png");
}

TEST(VPathSlashOpTest, AppendEmptyReturnsOriginal)
{
    VPath base("Assets://Textures");
    VPath result = base / "";
    EXPECT_EQ(result.ToString(), base.ToString());
}


// =============================================================================
// 숨김 파일 Extension/Stem 불일치 (BUG-4: Hidden File Handling)
// =============================================================================

TEST(VPathHiddenFileTest, DotFileExtensionConsistencyWithPath)
{
    // ".gitignore"는 숨김파일 — Path와 동일하게 확장자 없음, stem은 전체 파일명
    VPath vp("Assets://.gitignore");

    EXPECT_TRUE(vp.GetExtension().IsEmpty());
    EXPECT_EQ(vp.GetStem(), ".gitignore");
}

TEST(VPathHiddenFileTest, DotFileWithExtension)
{
    VPath vp("Assets://.bashrc.bak");

    // ".bashrc.bak" → stem=".bashrc", ext=".bak"
    EXPECT_EQ(vp.GetExtension(), ".bak");
    EXPECT_EQ(vp.GetStem(), ".bashrc");
}


// =============================================================================
// ParseAndNormalize 엣지 케이스
// =============================================================================

TEST(VPathNormalizationTest, DotDotInPathPartNotResolved)
{
    // VPath는 ".."을 해소하지 않음 (Path와 다르게)
    // 이 동작을 문서화하는 테스트
    VPath p("Assets://A/../B/file.txt");
    EXPECT_EQ(p.GetPathPart(), "A/../B/file.txt");
}

TEST(VPathNormalizationTest, MultipleSlashesNotCollapsed)
{
    // VPath는 이중 슬래시를 정리하지 않음
    VPath p("Assets://A//B///file.txt");
    EXPECT_EQ(p.GetPathPart(), "A//B///file.txt");
}

TEST(VPathNormalizationTest, ColonSequenceInPathPart)
{
    // 경로 부분에 "://"가 있는 경우 — 첫 번째 "://"만 스킴 구분자
    VPath p("Assets://path/with://colons/file.txt");
    EXPECT_EQ(p.GetScheme(), "Assets");
    EXPECT_EQ(p.GetPathPart(), "path/with://colons/file.txt");
}

TEST(VPathNormalizationTest, SchemeWithNumbers)
{
    VPath p("Cache2://data/file.bin");
    EXPECT_EQ(p.GetScheme(), "Cache2");
    EXPECT_TRUE(p.HasScheme());
}

TEST(VPathNormalizationTest, BackslashInScheme)
{
    // "Assets:\\" → 정규화 후 "Assets://" (스킴은 여전히 "Assets")
    VPath p("Assets:\\\\Textures\\file.png");
    EXPECT_EQ(p.GetScheme(), "Assets");
}


// =============================================================================
// uint16 오버플로우 테스트
// =============================================================================

TEST(VPathOverflowTest, VeryLongSchemeName)
{
    // 65535자를 초과하는 스킴명 (실제로는 비현실적이지만 안전성 검증)
    // uint16 한계 내에서의 동작 확인 (현실적 범위)
    String long_scheme(1000, 'A');  // 1000자 스킴
    String vpath_str = long_scheme + "://file.txt";

    VPath p(vpath_str);
    EXPECT_TRUE(p.HasScheme());
    EXPECT_EQ(p.GetScheme(), StringView(long_scheme));
}


// =============================================================================
// 유니코드 스킴 및 경로 (Unicode Scheme & Path)
// =============================================================================

TEST(VPathUnicodeTest, CJKFilenameComponents)
{
    VPath p("Assets://텍스처/플레이어_스프라이트.png");
    EXPECT_EQ(p.GetFilename(), "플레이어_스프라이트.png");
    EXPECT_EQ(p.GetStem(), "플레이어_스프라이트");
    EXPECT_EQ(p.GetExtension(), ".png");
}

TEST(VPathUnicodeTest, EmojiPath)
{
    VPath p("Assets://🎮/🎵.mp3");
    EXPECT_EQ(p.GetFilename(), "🎵.mp3");
    EXPECT_EQ(p.GetStem(), "🎵");
}

TEST(VPathUnicodeTest, MixedUnicodeAndAscii)
{
    VPath p("Assets://Models/キャラクター/hero_model.fbx");
    EXPECT_EQ(p.GetScheme(), "Assets");
    EXPECT_EQ(p.GetFilename(), "hero_model.fbx");
}


// =============================================================================
// GetParentPath 엣지 케이스
// =============================================================================

TEST(VPathParentTest, ParentOfSchemeOnlyReturnsInvalid)
{
    VPath p("Assets://");
    VPath parent = p.GetParentPath();
    // "Assets://"에서 마지막 '/'는 "://" 내부이므로 path_offset 이하
    // GetParentPath는 path_offset까지의 부분을 반환
    EXPECT_EQ(parent.ToString(), "Assets://");
}

TEST(VPathParentTest, ParentOfNoSchemePathWithoutSlash)
{
    VPath p("file.txt");
    VPath parent = p.GetParentPath();
    // 슬래시가 없으므로 빈 VPath
    EXPECT_FALSE(parent.IsValid());
}

TEST(VPathParentTest, ParentOfDeepNestedPath)
{
    VPath p("Assets://A/B/C/D/E/file.txt");
    VPath current = p;

    // 계층 순회: E → D → C → B → A → Assets://
    current = current.GetParentPath();
    EXPECT_EQ(current.ToString(), "Assets://A/B/C/D/E");

    current = current.GetParentPath();
    EXPECT_EQ(current.ToString(), "Assets://A/B/C/D");

    current = current.GetParentPath();
    EXPECT_EQ(current.ToString(), "Assets://A/B/C");

    current = current.GetParentPath();
    EXPECT_EQ(current.ToString(), "Assets://A/B");

    current = current.GetParentPath();
    EXPECT_EQ(current.ToString(), "Assets://A");

    current = current.GetParentPath();
    EXPECT_EQ(current.ToString(), "Assets://");
}
