#include "gtest/gtest.h"
#include "SimpleEngine/Core/Types/Path.h"

using namespace se;

TEST(PathTest, Constructors)
{
    Path p1;
    EXPECT_TRUE(p1.IsEmpty());

    Path p2("Engine/Source/Core.cpp");
    EXPECT_FALSE(p2.IsEmpty());

    String s = "Engine/Source/Main.cpp";
    Path p3(s);
    EXPECT_EQ(p3.ToString(), s);

    std::filesystem::path fs_path = "C:/Windows";
    Path p4(fs_path);
    EXPECT_EQ(p4.GetStdPath(), fs_path);
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

    Path p2("A/./B/../C");
    p2.Normalize();
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
    EXPECT_EQ(p2.GetNormalized().ToString(), "A/C");

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
