/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"

#include "absolutePath.h"
#include "absolutePathBuilder.h"

using namespace base;

DECLARE_TEST_FILE(AbsolutePath);

TEST(AbsolutePath, BuildEmpty)
{
    io::AbsolutePath emptyPath;
    ASSERT_TRUE(emptyPath.empty());
    ASSERT_EQ(0, wcslen(emptyPath.c_str()));
}

TEST(AbsolutePath, FiltersInvalidWindows)
{
    {
        auto path  = io::AbsolutePath::Build(L"Z:");
        EXPECT_TRUE(path.empty());
    }

    {
        auto path  = io::AbsolutePath::BuildAsDir(L"Z:\\");
        EXPECT_FALSE(path.empty());
    }

    {
        auto path  = io::AbsolutePath::Build(L"Z:\\crap");
        EXPECT_FALSE(path.empty());
    }

    {
        auto path  = io::AbsolutePath::BuildAsDir(L"Z:\\crap\\");
        EXPECT_FALSE(path.empty());
    }

    {
        auto path  = io::AbsolutePath::Build(L"Z:\\crap\\file.test");
        EXPECT_FALSE(path.empty());
    }

    {
        auto path  = io::AbsolutePath::Build(L"Z:\\crap\\file.test.");
        EXPECT_TRUE(path.empty());
    }

    {
        auto path  = io::AbsolutePath::Build(L"Z:\\crap\\file.test.test2");
        EXPECT_FALSE(path.empty());
    }
}

TEST(AbsolutePath, AppendEmptyDir)
{
    auto path  = io::AbsolutePath::Build(L"Z:\\test\\");
    path.appendDir(UTF16StringBuf(L""));

#ifdef PLATFORM_WINDOWS
    ASSERT_EQ(std::wstring(path.c_str()), L"Z:\\test\\");
#endif
}

TEST(AbsolutePath, AppendDirNormal)
{
    auto path  = io::AbsolutePath::Build(L"Z:\\test\\");
    path.appendDir(UTF16StringBuf(L"test1"));

#ifdef PLATFORM_WINDOWS
    ASSERT_EQ(std::wstring(path.c_str()), L"Z:\\test\\test1\\");
#endif
}

TEST(AbsolutePath, AppendDirWithSeparator)
{
    auto path  = io::AbsolutePath::Build(L"Z:\\test\\");
    path.appendDir(UTF16StringBuf(L"test1\\"));

#ifdef PLATFORM_WINDOWS
    ASSERT_EQ(std::wstring(path.c_str()), L"Z:\\test\\test1\\");
#endif
}

TEST(AbsolutePath, AppendMultipleDir)
{
    auto path  = io::AbsolutePath::Build(L"Z:\\test\\");
    path.appendDir(UTF16StringBuf(L"test1\\test2\\"));

#ifdef PLATFORM_WINDOWS
    ASSERT_EQ(std::wstring(path.c_str()), L"Z:\\test\\test1\\test2\\");
#endif
}

TEST(AbsolutePath, AppendEmptyFile)
{
    auto path  = io::AbsolutePath::Build(L"Z:\\test\\");
    path.appendFile(UTF16StringBuf(L""));

#ifdef PLATFORM_WINDOWS
    ASSERT_EQ(std::wstring(path.c_str()), L"Z:\\test\\");
#endif
}

TEST(AbsolutePath, AppendFile)
{
    auto path  = io::AbsolutePath::Build(L"Z:\\test\\");
    path.appendFile(UTF16StringBuf(L"test.txt"));

#ifdef PLATFORM_WINDOWS
    ASSERT_EQ(std::wstring(path.c_str()), L"Z:\\test\\test.txt");
#endif
}

TEST(AbsolutePath, RelativeTest)
{
    auto pathTest  = io::AbsolutePath::Build(L"Z:\\test\\crap\\test.txt");
    ASSERT_FALSE(pathTest.empty());
    auto pathBase  = io::AbsolutePath::Build(L"Z:\\test\\");
    ASSERT_FALSE(pathBase.empty());

    auto ret  = pathTest.relativeTo(pathBase);

#ifdef PLATFORM_WINDOWS
    auto testPath  = std::wstring(L"crap\\test.txt");
#else
    auto testPath  = std::wstring(L"crap/test.txt");
#endif
    ASSERT_EQ(std::wstring(ret.c_str()), testPath);
}

TEST(AbsolutePath, RelativeTestSame)
{
    auto pathTest  = io::AbsolutePath::Build(L"Z:\\test\\crap\\test.txt");
    ASSERT_FALSE(pathTest.empty());
    auto pathBase  = io::AbsolutePath::Build(L"Z:\\test\\crap\\test.txt");
    ASSERT_FALSE(pathBase.empty());

    auto ret  = pathTest.relativeTo(pathBase);
    ASSERT_TRUE(ret.empty());
}

TEST(AbsolutePath, RelativeTestSameInvalid)
{
    auto pathTest  = io::AbsolutePath::Build(L"Z:\\test\\crap\\test.txt");
    ASSERT_FALSE(pathTest.empty());
    auto pathBase  = io::AbsolutePath::Build(L"Z:\\test2\\");
    ASSERT_FALSE(pathBase.empty());

    auto ret  = pathTest.relativeTo(pathBase);
    ASSERT_FALSE(ret.empty());
    auto finalPath  = pathBase.appendFile(ret);
    ASSERT_EQ(pathTest, finalPath);
}

TEST(AbsolutePath, ComplexPathBuild)
{
    auto pathTest  = io::AbsolutePath::Build(L"Z:\\test\\crap\\..\\test.txt");
    ASSERT_FALSE(pathTest.empty());
}

TEST(AbsolutePath, ComplexPathAutoNormalizes)
{
    auto pathTest  = io::AbsolutePath::Build(L"Z:\\test\\crap\\..\\test.txt");
    auto pathTest2 = io::AbsolutePath::Build(L"Z:\\test\\test.txt");

    EXPECT_EQ(std::wstring(pathTest.c_str()), std::wstring(pathTest2.c_str()));
}

TEST(AbsolutePath, PushingDirOnFileFails)
{
    auto pathTest = io::AbsolutePath::Build(L"Z:\\test\\crap\\");
    auto pathTest2 = io::AbsolutePath::Build(L"Z:\\test\\");

    pathTest.appendDir(UTF16StringBuf(L".."));

    EXPECT_EQ(std::wstring(pathTest.c_str()), std::wstring(pathTest2.c_str()));
}

//---

TEST(AbsolutePathBuilder, SimplePath)
{
    auto pathTest = io::AbsolutePath::Build(L"Z:\\test\\crap\\file.txt.xml");
    io::AbsolutePathBuilder builder(pathTest);

    EXPECT_EQ(std::wstring(builder.drive().c_str()), L"Z:");

    ASSERT_EQ(2, builder.numDirectories());
    EXPECT_EQ(std::wstring(builder.directory(0).c_str()), L"test");
    EXPECT_EQ(std::wstring(builder.directory(1).c_str()), L"crap");

    EXPECT_EQ(std::wstring(builder.fileName().c_str()), L"file");

    ASSERT_EQ(2, builder.numExtensions());
    //EXPECT_EQ(std::wstring(builder.extension(0).c_str()), L"txt");
    //EXPECT_EQ(std::wstring(builder.extension(1).c_str()), L"xml");

    EXPECT_EQ(std::wstring(builder.mainExtension().c_str()), L"xml");

    auto reassembledPath = builder.toAbsolutePath();
    EXPECT_EQ(std::wstring(pathTest.c_str()), std::wstring(reassembledPath.c_str()));
}

TEST(AbsolutePathBuilder, NormalizedPath)
{
    auto pathTest = io::AbsolutePath::Build(L"Z:\\test\\crap\\..\\.\\..\\\\file.txt.xml");
    io::AbsolutePathBuilder builder(pathTest);

    EXPECT_EQ(std::wstring(builder.drive().c_str()), L"Z:");
    ASSERT_EQ(0, builder.numDirectories());
    EXPECT_EQ(std::wstring(builder.fileName().c_str()), L"file");
}

//---

TEST(AbsolutePath, FiltersInvalidLinux)
{
    {
        auto path = io::AbsolutePath::Build(L"~");
        EXPECT_TRUE(path.empty());
    }

    {
        auto path = io::AbsolutePath::Build(L"~/");
        EXPECT_TRUE(path.empty());
    }

    {
        auto path = io::AbsolutePath::Build(L"/");
        EXPECT_FALSE(path.empty());
    }

    {
        auto path = io::AbsolutePath::Build(L"\\");
        EXPECT_FALSE(path.empty());
    }

    {
        auto path = io::AbsolutePath::Build(L"/crap");
        EXPECT_FALSE(path.empty());
    }

    {
        auto path = io::AbsolutePath::Build(L"/crap/");
        EXPECT_FALSE(path.empty());
    }

    {
        auto path = io::AbsolutePath::Build(L"/crap/file.test");
        EXPECT_FALSE(path.empty());
    }

    {
        auto path = io::AbsolutePath::Build(L"/crap/file.test.");
        EXPECT_TRUE(path.empty());
    }

    {
        auto path = io::AbsolutePath::Build(L"/crap/file.test.test2");
        EXPECT_FALSE(path.empty());
    }
}

TEST(AbsolutePath, AppendEmptyDirLinux)
{
    auto path = io::AbsolutePath::Build(L"/test/");
    path.appendDir(UTF16StringBuf(L""));

    auto testPath = io::AbsolutePath::Build(L"/test/");
    ASSERT_EQ(std::wstring(path.c_str()), std::wstring(testPath.c_str()));
}

TEST(AbsolutePath, AppendDirNormalLinux)
{
    auto path = io::AbsolutePath::Build(L"/test/");
    path.appendDir(UTF16StringBuf(L"test1"));

    auto testPath = io::AbsolutePath::Build(L"/test/test1/");
    ASSERT_EQ(std::wstring(path.c_str()), std::wstring(testPath.c_str()));
}

TEST(AbsolutePath, AppendDirWithSeparatorLinux)
{
    auto path = io::AbsolutePath::Build(L"/test/");
    path.appendDir(UTF16StringBuf(L"test1/"));

    auto testPath = io::AbsolutePath::Build(L"/test/test1/");
    ASSERT_EQ(std::wstring(path.c_str()), std::wstring(testPath.c_str()));
}

TEST(AbsolutePath, AppendMultipleDirLinux)
{
    auto path = io::AbsolutePath::Build(L"/test/");
    path.appendDir(UTF16StringBuf(L"/test1/test2/"));

    auto testPath = io::AbsolutePath::Build(L"/test/test1/test2/");
    ASSERT_EQ(std::wstring(path.c_str()), std::wstring(testPath.c_str()));
}

TEST(AbsolutePath, AppendEmptyFileLinux)
{
    auto path = io::AbsolutePath::Build(L"/test/");
    path.appendFile(UTF16StringBuf(L""));

    auto testPath = io::AbsolutePath::Build(L"/test/");
    ASSERT_EQ(std::wstring(path.c_str()), std::wstring(testPath.c_str()));
}

TEST(AbsolutePath, AppendFileLinux)
{
    auto path = io::AbsolutePath::Build(L"/test/");
    path.appendFile(UTF16StringBuf(L"test.txt"));

    auto testPath = io::AbsolutePath::Build(L"/test/test.txt");
    ASSERT_EQ(std::wstring(path.c_str()), std::wstring(testPath.c_str()));
}

TEST(AbsolutePath, RelativeTestLinux)
{
    auto pathTest = io::AbsolutePath::Build(L"/test/crap/test.txt");
    ASSERT_FALSE(pathTest.empty());
    auto pathBase = io::AbsolutePath::Build(L"/test/");
    ASSERT_FALSE(pathBase.empty());

    auto ret = pathTest.relativeTo(pathBase);

#ifdef PLATFORM_WINDOWS
    auto testPath = std::wstring(L"crap\\test.txt");
#else
    auto testPath = std::wstring(L"crap/test.txt");
#endif
    ASSERT_EQ(std::wstring(ret.c_str()), std::wstring(testPath.c_str()));
}

TEST(AbsolutePath, RelativeTestSameLinux)
{
    auto pathTest = io::AbsolutePath::Build(L"/test/crap/test.txt");
    ASSERT_FALSE(pathTest.empty());
    auto pathBase = io::AbsolutePath::Build(L"/test/crap/test.txt");
    ASSERT_FALSE(pathBase.empty());

    auto ret = pathTest.relativeTo(pathBase);
    ASSERT_TRUE(ret.empty());
}

TEST(AbsolutePath, RelativeTestSameInvalidLinux)
{
    auto pathTest = io::AbsolutePath::Build(L"/test/crap/test.txt");
    ASSERT_FALSE(pathTest.empty());
    auto pathBase = io::AbsolutePath::Build(L"/test2/");
    ASSERT_FALSE(pathBase.empty());

    auto ret = pathTest.relativeTo(pathBase);
    ASSERT_FALSE(ret.empty());

    auto finalPath = pathBase.appendFile(ret);
    ASSERT_EQ(pathTest, finalPath);
}

TEST(AbsolutePath, ComplexPathBuildLinux)
{
    auto pathTest = io::AbsolutePath::Build(L"/test/crap/../test.txt");
    ASSERT_FALSE(pathTest.empty());
}

TEST(AbsolutePath, ComplexPathAutoNormalizesLinux)
{
    auto pathTest = io::AbsolutePath::Build(L"/test/crap/../test.txt");
    auto pathTest2 = io::AbsolutePath::Build(L"/test/test.txt");

    EXPECT_EQ(std::wstring(pathTest.c_str()), std::wstring(pathTest2.c_str()));
}

TEST(AbsolutePath, PushingDirOnFileFailsLinux)
{
    auto pathTest = io::AbsolutePath::Build(L"/test/crap/");
    auto pathTest2 = io::AbsolutePath::Build(L"/test/");

    pathTest.appendDir(UTF16StringBuf(L".."));

    EXPECT_EQ(std::wstring(pathTest.c_str()), std::wstring(pathTest2.c_str()));
}

//---

TEST(AbsolutePathBuilder, SimplePathLinux)
{
    auto pathTest = io::AbsolutePath::Build(L"/test/crap/file.txt.xml");
    io::AbsolutePathBuilder builder(pathTest);

    EXPECT_EQ(std::wstring(builder.drive().c_str()), L"");

    ASSERT_EQ(2, builder.numDirectories());
    EXPECT_EQ(std::wstring(builder.directory(0).c_str()), L"test");
    EXPECT_EQ(std::wstring(builder.directory(1).c_str()), L"crap");

    EXPECT_EQ(std::wstring(builder.fileName().c_str()), L"file");

    ASSERT_EQ(2, builder.numExtensions());
    //EXPECT_EQ(std::wstring(builder.extension(0).c_str()), L"txt");
    //EXPECT_EQ(std::wstring(builder.extension(1).c_str()), L"xml");

    EXPECT_EQ(std::wstring(builder.mainExtension().c_str()), L"xml");

    auto reassembledPath = builder.toAbsolutePath();
    EXPECT_EQ(std::wstring(pathTest.c_str()), std::wstring(reassembledPath.c_str()));
}

TEST(AbsolutePathBuilder, NormalizedPathLinux)
{
    auto pathTest = io::AbsolutePath::Build(L"/test/crap/.././..//file.txt.xml");
    io::AbsolutePathBuilder builder(pathTest);

    EXPECT_EQ(std::wstring(builder.drive().c_str()), L"");
    ASSERT_EQ(0, builder.numDirectories());
    EXPECT_EQ(std::wstring(builder.fileName().c_str()), L"file");
}
