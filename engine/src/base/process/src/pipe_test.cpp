/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"
#include "base/process/include/pipe.h"
#include "base/containers/include/uniquePtr.h"

#include "base/test/include/gtest/gtest.h"

DECLARE_TEST_FILE(Pipes);

typedef base::UniquePtr<base::process::IPipeWriter> PipeWriterPtr;
typedef base::UniquePtr<base::process::IPipeReader> PipeRederPtr;

TEST(Pipes, CreateNamedPipeWriteEnd)
{
    PipeWriterPtr pipe(base::process::IPipeWriter::Create());
    ASSERT_NE(nullptr, pipe.get());
}

TEST(Pipes, CreateNamedPipeReadEnd)
{
    PipeRederPtr pipe(base::process::IPipeReader::Create());
    ASSERT_NE(nullptr, pipe.get());
}

TEST(Pipes, CanOpenExistingWriterPipe)
{
    PipeWriterPtr pipe(base::process::IPipeWriter::Create());
    ASSERT_NE(nullptr, pipe.get());

    PipeRederPtr pipe2(base::process::IPipeReader::Open(pipe->name()));
    ASSERT_NE(nullptr, pipe2.get());
}

TEST(Pipes, CanOpenExistingReaderPipe)
{
    PipeRederPtr pipe(base::process::IPipeReader::Create());
    ASSERT_NE(nullptr, pipe.get());

    PipeWriterPtr pipe2(base::process::IPipeWriter::Open(pipe->name()));
    ASSERT_NE(nullptr, pipe2.get());
}

TEST(Pipes, SendFragmentedData)
{
    PipeRederPtr pipe(base::process::IPipeReader::Create());
    ASSERT_NE(nullptr, pipe.get());

    PipeWriterPtr pipe2(base::process::IPipeWriter::Open(pipe->name()));
    ASSERT_NE(nullptr, pipe2.get());

    const char* txt = "Ala ma kota";
    auto len = strlen(txt) + 1;

    for (size_t i = 0; i < len; ++i)
    {
        auto sent = pipe2->write(txt + i, 1);
        EXPECT_EQ(1, sent);
    }

    char buf[512] = { 0 };
    size_t total = 0;
    while (total <= len)
    {
        auto r = pipe->read(buf + total, (uint32_t)(sizeof(buf) - total));
        if (r == 0)
            break;
        total += r;
    }

    EXPECT_EQ(0, strcmp(buf, txt));
}
