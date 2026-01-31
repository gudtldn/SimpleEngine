#include "gtest/gtest.h"

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
    std::string_view sv = "Assets://Textures/Player.png";
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
    EXPECT_TRUE(p.GetScheme().empty());
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
    EXPECT_TRUE(p.GetFilename().empty());
}

TEST(VPathTest, GetExtensionReturnsExtension)
{
    VPath p("Assets://Textures/Player.png");
    EXPECT_EQ(p.GetExtension(), ".png");
}

TEST(VPathTest, GetExtensionReturnsEmptyForNoExtension)
{
    VPath p("Assets://Textures/README");
    EXPECT_TRUE(p.GetExtension().empty());
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
    EXPECT_TRUE(p.GetPathPart().empty());
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
