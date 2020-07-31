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
#include "ioFileHandle.h"
#include "ioSystem.h"

DECLARE_TEST_FILE(FileAccess);

using namespace base;

TEST(FileAccess, ExePathValid)
{
    auto path = base::io::SystemPath(base::io::PathCategory::ExecutableFile);
    ASSERT_FALSE(path.empty());
}

TEST(FileAccess, FileExistsTest)
{
    auto path = base::io::SystemPath(base::io::PathCategory::ExecutableFile);
    ASSERT_TRUE(base::io::FileExists(path));
}

TEST(FileAccess, FileExistsTestFalse)
{
    auto path = base::io::SystemPath(base::io::PathCategory::ExecutableFile).addExtension("test");
    ASSERT_FALSE(base::io::FileExists(path));
}

TEST(FileAccess, CachePathValid)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir);
    ASSERT_FALSE(path.empty());
}

TEST(FileAccess, CreateCacheFile)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir).addFile(L"cache.txt");
    auto f = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);
    f.reset();
    ASSERT_TRUE(base::io::FileExists(path));
    ASSERT_TRUE(base::io::DeleteFile(path));
    ASSERT_FALSE(base::io::FileExists(path));
}

TEST(FileAccess, WriteFile)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir).addFile(L"cache.txt");
    base::io::DeleteFile(path);

    auto f = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);
    ASSERT_TRUE(f->pos() == 0);

    const char* test = "Ala ma kota";
    auto toWrite = strlen(test);
    auto written = f->writeSync(test, toWrite);
    ASSERT_TRUE(f->pos() == toWrite);
    f.reset();

    ASSERT_TRUE(base::io::FileExists(path));

    uint64_t fileSize = 0;
    base::io::FileSize(path, fileSize);
    ASSERT_EQ(toWrite, fileSize);
    ASSERT_TRUE(base::io::DeleteFile(path));
    ASSERT_FALSE(base::io::FileExists(path));
}

#ifdef PLATFORM_WINDOWS

TEST(FileAccess, WriteFileExclusiveForWrite)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    auto f = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f2.get() == nullptr);

    f.reset();

    auto f3 = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f3.get() != nullptr);

    f3.reset();

    ASSERT_TRUE(base::io::DeleteFile(path));
}

TEST(FileAccess, WriteFileExclusiveForRead)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    auto f = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = base::io::OpenForReading(path);
    ASSERT_TRUE(f2.get() == nullptr);

    f.reset();

    auto f3 = base::io::OpenForReading(path);
    ASSERT_TRUE(f3.get() != nullptr);

    f3.reset();

    ASSERT_TRUE(base::io::DeleteFile(path));
}

TEST(FileAccess, WriteFileExclusiveForDelete)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    auto f = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);

    ASSERT_FALSE(base::io::DeleteFile(path));

    f.reset();

    ASSERT_TRUE(base::io::DeleteFile(path));
}

TEST(FileAccess, ReadFileExclusiveForWrite)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    {
        auto f3 = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
        f3.reset();
    }

    auto f = base::io::OpenForReading(path);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f2.get() == nullptr);

    f.reset();

    auto f3 = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f3.get() != nullptr);

    f3.reset();

    ASSERT_TRUE(base::io::DeleteFile(path));
}

#endif

TEST(FileAccess, ReadFileNonExclusiveForRead)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    {
        auto f3 = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
        f3.reset();
    }

    auto f = base::io::OpenForReading(path);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = base::io::OpenForReading(path);
    ASSERT_TRUE(f2.get() != nullptr);

    f.reset();
    f2.reset();

    ASSERT_TRUE(base::io::DeleteFile(path));
}


TEST(FileAccess, ReadFile)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));
    base::io::DeleteFile(path);

    {
        auto f = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_TRUE(f->pos() == 0);

        const char* test = "Ala ma kota";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_TRUE(f->pos() == toWrite);
        f.reset();
    }

    {
        auto f = base::io::OpenForReading(path);
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

    ASSERT_TRUE(base::io::DeleteFile(path));
    ASSERT_FALSE(base::io::FileExists(path));
}

TEST(FileAccess, AppendFile)
{
    auto path = base::io::SystemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));
    base::io::DeleteFile(path);

    {
        auto f = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectWrite);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_EQ(0, f->pos());

        const char* test = "Ala ";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_EQ(toWrite, f->pos());
        f.reset();
    }

    {
        auto f = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectAppend);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_EQ(4, f->pos());

        const char* test = "ma ";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_EQ(toWrite + 4, f->pos());
        f.reset();
    }

    {
        auto f = base::io::OpenForWriting(path, base::io::FileWriteMode::DirectAppend);
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
        auto f = base::io::OpenForReading(path);
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

    ASSERT_TRUE(base::io::DeleteFile(path));
    ASSERT_FALSE(base::io::FileExists(path));
}
