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
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::ExecutableFile);
    ASSERT_FALSE(path.empty());
}

TEST(FileAccess, FileExistsTest)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::ExecutableFile);
    ASSERT_TRUE(IO::GetInstance().fileExists(path));
}

TEST(FileAccess, FileExistsTestFalse)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::ExecutableFile).addExtension("test");
    ASSERT_FALSE(IO::GetInstance().fileExists(path));
}

TEST(FileAccess, CachePathValid)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir);
    ASSERT_FALSE(path.empty());
}

TEST(FileAccess, CreateCacheFile)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addFile(L"cache.txt");
    auto f = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);
    f.reset();
    ASSERT_TRUE(IO::GetInstance().fileExists(path));
    ASSERT_TRUE(IO::GetInstance().deleteFile(path));
    ASSERT_FALSE(IO::GetInstance().fileExists(path));
}

TEST(FileAccess, WriteFile)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addFile(L"cache.txt");
    IO::GetInstance().deleteFile(path);

    auto f = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);
    ASSERT_TRUE(f->pos() == 0);

    const char* test = "Ala ma kota";
    auto toWrite = strlen(test);
    auto written = f->writeSync(test, toWrite);
    ASSERT_TRUE(f->pos() == toWrite);
    f.reset();

    ASSERT_TRUE(IO::GetInstance().fileExists(path));

    uint64_t fileSize = 0;
    IO::GetInstance().fileSize(path, fileSize);
    ASSERT_EQ(toWrite, fileSize);
    ASSERT_TRUE(IO::GetInstance().deleteFile(path));
    ASSERT_FALSE(IO::GetInstance().fileExists(path));
}

#ifdef PLATFORM_WINDOWS

TEST(FileAccess, WriteFileExclusiveForWrite)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    auto f = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f2.get() == nullptr);

    f.reset();

    auto f3 = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f3.get() != nullptr);

    f3.reset();

    ASSERT_TRUE(IO::GetInstance().deleteFile(path));
}

TEST(FileAccess, WriteFileExclusiveForRead)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    auto f = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = IO::GetInstance().openForReading(path);
    ASSERT_TRUE(f2.get() == nullptr);

    f.reset();

    auto f3 = IO::GetInstance().openForReading(path);
    ASSERT_TRUE(f3.get() != nullptr);

    f3.reset();

    ASSERT_TRUE(IO::GetInstance().deleteFile(path));
}

TEST(FileAccess, WriteFileExclusiveForDelete)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    auto f = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f.get() != nullptr);

    ASSERT_FALSE(IO::GetInstance().deleteFile(path));

    f.reset();

    ASSERT_TRUE(IO::GetInstance().deleteFile(path));
}

TEST(FileAccess, ReadFileExclusiveForWrite)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    {
        auto f3 = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
        f3.reset();
    }

    auto f = IO::GetInstance().openForReading(path);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f2.get() == nullptr);

    f.reset();

    auto f3 = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
    ASSERT_TRUE(f3.get() != nullptr);

    f3.reset();

    ASSERT_TRUE(IO::GetInstance().deleteFile(path));
}

#endif

TEST(FileAccess, ReadFileNonExclusiveForRead)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));

    {
        auto f3 = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
        f3.reset();
    }

    auto f = IO::GetInstance().openForReading(path);
    ASSERT_TRUE(f.get() != nullptr);

    auto f2 = IO::GetInstance().openForReading(path);
    ASSERT_TRUE(f2.get() != nullptr);

    f.reset();
    f2.reset();

    ASSERT_TRUE(IO::GetInstance().deleteFile(path));
}


TEST(FileAccess, ReadFile)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));
    IO::GetInstance().deleteFile(path);

    {
        auto f = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_TRUE(f->pos() == 0);

        const char* test = "Ala ma kota";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_TRUE(f->pos() == toWrite);
        f.reset();
    }

    {
        auto f = IO::GetInstance().openForReading(path);
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

    ASSERT_TRUE(IO::GetInstance().deleteFile(path));
    ASSERT_FALSE(IO::GetInstance().fileExists(path));
}

TEST(FileAccess, AppendFile)
{
    auto path = IO::GetInstance().systemPath(base::io::PathCategory::TempDir).addFile(UTF16StringBuf(L"cache.txt"));
    IO::GetInstance().deleteFile(path);

    {
        auto f = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectWrite);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_EQ(0, f->pos());

        const char* test = "Ala ";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_EQ(toWrite, f->pos());
        f.reset();
    }

    {
        auto f = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectAppend);
        ASSERT_TRUE(f.get() != nullptr);
        ASSERT_EQ(4, f->pos());

        const char* test = "ma ";
        auto toWrite = strlen(test);
        auto written = f->writeSync(test, toWrite);

        ASSERT_EQ(toWrite + 4, f->pos());
        f.reset();
    }

    {
        auto f = IO::GetInstance().openForWriting(path, base::io::FileWriteMode::DirectAppend);
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
        auto f = IO::GetInstance().openForReading(path);
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

    ASSERT_TRUE(IO::GetInstance().deleteFile(path));
    ASSERT_FALSE(IO::GetInstance().fileExists(path));
}
