/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "core/test/include/gtest/gtest.h"

#include "ioFileHandle.h"
#include "ioSystem.h"

DECLARE_TEST_FILE(FileAccess);

BEGIN_BOOMER_NAMESPACE()

TEST(FileAccess, ExePathValid)
{
    auto path = io::SystemPath(io::PathCategory::ExecutableFile);
    ASSERT_FALSE(path.empty());
}

TEST(FileAccess, FileExistsTest)
{
    auto path = io::SystemPath(io::PathCategory::ExecutableFile);
    ASSERT_TRUE(io::FileExists(path));
}

TEST(FileAccess, FileExistsTestFalse)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::ExecutableFile);
    path << ".text";
    ASSERT_FALSE(io::FileExists(path.view()));
}

TEST(FileAccess, CachePathValid)
{
    auto path = io::SystemPath(io::PathCategory::LocalTempDir);
    ASSERT_FALSE(path.empty());
}

TEST(FileAccess, CreateCacheFile)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::LocalTempDir);
    path << "cache.txt";

    auto f = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);
    f.reset();
    ASSERT_TRUE(io::FileExists(path.view()));
    ASSERT_TRUE(io::DeleteFile(path.view()));
    ASSERT_FALSE(io::FileExists(path.view()));
}

TEST(FileAccess, WriteFile)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::LocalTempDir);
    path << "cache.txt";

    io::DeleteFile(path.view());

    auto f = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);
    ASSERT_TRUE(f->pos() == 0);

    const char* test = "Ala ma kota";
    auto toWrite = strlen(test);
    auto written = f->writeSync(test, toWrite);
    ASSERT_TRUE(f->pos() == toWrite);
    f.reset();

    ASSERT_TRUE(io::FileExists(path.view()));

    uint64_t fileSize = 0;
    io::FileSize(path.view(), fileSize);
    ASSERT_EQ(toWrite, fileSize);
    ASSERT_TRUE(io::DeleteFile(path.view()));
    ASSERT_FALSE(io::FileExists(path.view()));
}

#ifdef PLATFORM_WINDOWS

TEST(FileAccess, WriteFileExclusiveForWrite)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::LocalTempDir);
    path << "cache.txt";

    auto f = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f2.get() == nullptr);

    f.reset();

    auto f3 = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f3.get() != nullptr);

    f3.reset();

    ASSERT_TRUE(io::DeleteFile(path.view()));
}

TEST(FileAccess, WriteFileExclusiveForRead)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::LocalTempDir);
    path << "cache.txt";

    auto f = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = io::OpenForReading(path.view());
    ASSERT_TRUE(f2.get() == nullptr);

    f.reset();

    auto f3 = io::OpenForReading(path.view());
    ASSERT_TRUE(f3.get() != nullptr);

    f3.reset();

    ASSERT_TRUE(io::DeleteFile(path.view()));
}

TEST(FileAccess, WriteFileExclusiveForDelete)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::LocalTempDir);
    path << "cache.txt";

    auto f = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);

    ASSERT_FALSE(io::DeleteFile(path.view()));

    f.reset();

    ASSERT_TRUE(io::DeleteFile(path.view()));
}

TEST(FileAccess, ReadFileExclusiveForWrite)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::LocalTempDir);
    path << "cache.txt";

    {
        auto f3 = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
        f3.reset();
    }

    auto f = io::OpenForReading(path.view());
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f2.get() == nullptr);

    f.reset();

    auto f3 = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f3.get() != nullptr);

    f3.reset();

    ASSERT_TRUE(io::DeleteFile(path.view()));
}

#endif

TEST(FileAccess, ReadFileNonExclusiveForRead)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::LocalTempDir);
    path << "cache.txt";

    {
        auto f3 = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
        f3.reset();
    }

    auto f = io::OpenForReading(path.view());
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = io::OpenForReading(path.view());
    ASSERT_TRUE(f2.get() != nullptr);

    f.reset();
    f2.reset();

    ASSERT_TRUE(io::DeleteFile(path.view()));
}


TEST(FileAccess, ReadFile)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::LocalTempDir);
    path << "cache.txt";

    io::DeleteFile(path.view());

    {
        auto f = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_TRUE(f->pos() == 0);

        const char* test = "Ala ma kota";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_TRUE(f->pos() == toWrite);
        f.reset();
    }

    {
        auto f = io::OpenForReading(path.view());
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_EQ(11, f->size());
        ASSERT_EQ(0, f->pos());

        char buf[100];
        auto read = f->readSync(buf, range_cast<size_t>(f->size()));
        buf[f->size()] = 0;
        ASSERT_EQ(read, f->size());

        ASSERT_EQ(std::string("Ala ma kota"), std::string(buf));
        f.reset();
    }

    ASSERT_TRUE(io::DeleteFile(path.view()));
    ASSERT_FALSE(io::FileExists(path.view()));
}

TEST(FileAccess, AppendFile)
{
    StringBuilder path;
    path << io::SystemPath(io::PathCategory::LocalTempDir);
    path << "cache.txt";

    io::DeleteFile(path.view());

    {
        auto f = io::OpenForWriting(path.view(), io::FileWriteMode::DirectWrite);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_EQ(0, f->pos());

        const char* test = "Ala ";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_EQ(toWrite, f->pos());
        f.reset();
    }

    {
        auto f = io::OpenForWriting(path.view(), io::FileWriteMode::DirectAppend);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_EQ(4, f->pos());

        const char* test = "ma ";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_EQ(toWrite + 4, f->pos());
        f.reset();
    }

    {
        auto f = io::OpenForWriting(path.view(), io::FileWriteMode::DirectAppend);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_EQ(7, f->pos());

        const char* test = "kota";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_EQ(11, f->pos());
        ASSERT_EQ(11, f->size());
        f.reset();
    }

    {
        auto f = io::OpenForReading(path.view());
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_EQ(11, f->size());
        ASSERT_EQ(0, f->pos());

        char buf[100];
        auto read = f->readSync(buf, range_cast<size_t>(f->size()));
        buf[f->size()] = 0;
        ASSERT_EQ(read, f->size());

        ASSERT_EQ(std::string("Ala ma kota"), std::string(buf));
        f.reset();
    }

    ASSERT_TRUE(io::DeleteFile(path.view()));
    ASSERT_FALSE(io::FileExists(path.view()));
}

END_BOOMER_NAMESPACE()
