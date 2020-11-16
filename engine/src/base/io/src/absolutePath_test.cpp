/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"

#include "pathBuilder.h"

using namespace base;

DECLARE_TEST_FILE(PathTests);


//---

TEST(PathBuilder, SimplePathLinux)
{
    io::PathBuilder builder("/test/crap/file.txt.xml");

    EXPECT_EQ(std::string(builder.drive().c_str()), "");

    ASSERT_EQ(2, builder.numDirectories());
    EXPECT_STREQ(builder.directory(0).c_str(), "test");
    EXPECT_STREQ(builder.directory(1).c_str(), "crap");

    EXPECT_STREQ(builder.fileName().c_str(), "file");

    ASSERT_EQ(2, builder.numExtensions());
    EXPECT_STREQ(builder.specificExtension(0).c_str(), "txt");
    EXPECT_STREQ(builder.specificExtension(1).c_str(), "xml");

    EXPECT_STREQ(builder.mainExtension().c_str(), "xml");

    auto reassembledPath = builder.toString();
    EXPECT_STREQ("/test/crap/file.txt.xml", reassembledPath.c_str());
}

TEST(PathBuilder, NormalizedPathLinux)
{
    io::PathBuilder builder("/test/crap/.././..//file.txt.xml");

    EXPECT_STREQ(builder.drive().c_str(), "");
    ASSERT_EQ(0, builder.numDirectories());
    EXPECT_STREQ(builder.fileName().c_str(), "file");
}
