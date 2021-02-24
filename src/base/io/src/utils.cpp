/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"

#include "ioFileHandle.h"
#include "ioSystem.h"

#include "base/memory/include/buffer.h"

BEGIN_BOOMER_NAMESPACE(base::io)

//--

Buffer AllocBlockAsync(uint32_t size)
{
    ASSERT(!"TODO");
    return Buffer();
}

bool AllocBlocksAsync(uint32_t compressedSize, uint32_t decompressedSize, Buffer& outCompressedBuffer, Buffer& outDecompressedBuffer)
{
    ASSERT(!"TODO");
    return false;
}

//--

bool LoadFileToString(StringView absoluteFilePath, StringBuf &outString)
{
    // load content to memory buffer
    auto buffer = LoadFileToBuffer(absoluteFilePath);
    if (!buffer)
        return false;

    // create string
    outString = StringBuf(buffer.data(), buffer.size());
    return true;
}

bool LoadFileToArray(StringView absoluteFilePath, Array<uint8_t>& outArray)
{
    auto file  = OpenForReading(absoluteFilePath);
    if (!file)
        return false;

    auto size  = file->size();
    if (size > (uint64_t)std::numeric_limits<uint32_t>::max())
        return false;

    if (size == 0)
        return false;

    Array<uint8_t> array;
    array.resize((uint32_t)size);
    if (!array.data())
        return false;

    if (file->readSync(array.data(), array.dataSize()) != size)
        return false;

    outArray = std::move(array);
    return true;
}

bool SaveFileFromString(StringView absoluteFilePath, StringView str, StringEncoding encoding /*= StringEncoding::UTF8*/)
{
    // open file
    auto file  = OpenForWriting(absoluteFilePath);
    if (!file)
        return false;

    // always save as ansi
    auto size  = str.length();
    auto written  = file->writeSync(str.data(), size);
    if (written != size)
    {
        TRACE_ERROR("Written {} bytes instead of {} to '{}'. Failed to save string to file.", written, size, absoluteFilePath);
        return false;
    }

    // saved
    return true;
}

bool SaveFileFromBuffer(StringView absoluteFilePath, const void* buffer, size_t size)
{
    // open file
    auto file  = OpenForWriting(absoluteFilePath);
    if (!file)
        return false;

    // write data
    auto written  = file->writeSync(buffer, size);
    if (written != size)
    {
        TRACE_ERROR("Written {} bytes instead of {} to '{}'. Failed to save buffer to file.", written, size, absoluteFilePath);
        return false;
    }

    return true;
}

bool SaveFileFromBuffer(StringView absoluteFilePath, const Buffer& buffer)
{
    return SaveFileFromBuffer(absoluteFilePath, buffer.data(), buffer.size());
}

//--

END_BOOMER_NAMESPACE(base::io)
