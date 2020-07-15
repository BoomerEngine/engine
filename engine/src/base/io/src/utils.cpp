/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "absolutePath.h"
#include "utils.h"

#include "ioFileHandle.h"
#include "ioSystem.h"

#include "base/memory/include/buffer.h"

namespace base
{
    namespace io
    {
        bool LoadFileToString(const AbsolutePath& absoluteFilePath, StringBuf &outString)
        {
            // load content to memory buffer
            auto buffer = LoadFileToBuffer(absoluteFilePath);
            if (!buffer)
                return false;

            // create string
            outString = StringBuf(buffer.data(), buffer.size());
            return true;
        }

        bool LoadFileToArray(const AbsolutePath& absoluteFilePath, Array<uint8_t>& outArray)
        {
            auto file  = IO::GetInstance().openForReading(absoluteFilePath);
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

        Buffer LoadFileToBuffer(const AbsolutePath& absoluteFilePath)
        {
            auto file  = IO::GetInstance().openForReading(absoluteFilePath);
            if (!file)
                return nullptr;

            // prepare buffer
            const auto fileSize = file->size();
            auto ret = Buffer::Create(POOL_IO, fileSize);
            if (ret)
            {
                auto readSize = file->readSync(ret.data(), fileSize);
                if (readSize != fileSize)
                {
                    TRACE_ERROR("Read {} bytes instead of {} from '{}'. Failed to load file to buffer.", readSize, fileSize, absoluteFilePath.c_str());
                    ret.reset();
                }
            }
            else
            {
                TRACE_WARNING("Failed to allocate buffer for reading content of file '{}', required {}", absoluteFilePath, MemSize(fileSize));
            }

            return ret;
        }

        bool SaveFileFromString(const AbsolutePath& absoluteFilePath, StringView<char> str)
        {
            // open file
            auto file  = IO::GetInstance().openForWriting(absoluteFilePath);
            if (!file)
                return false;

            // always save as ansi
            auto size  = str.length();
            auto written  = file->writeSync(str.data(), size);
            if (written != size)
            {
                TRACE_ERROR("Written {} bytes instead of {} to '{}'. Failed to save string to file.",
                    written, size, absoluteFilePath.c_str());
                return false;
            }

            // saved
            return true;
        }

        bool SaveFileFromBuffer(const AbsolutePath& absoluteFilePath, const void* buffer, size_t size)
        {
            // open file
            auto file  = IO::GetInstance().openForWriting(absoluteFilePath);
            if (!file)
                return false;

            // write data
            auto written  = file->writeSync(buffer, size);
            if (written != size)
            {
                TRACE_ERROR("Written {} bytes instead of {} to '{}'. Failed to save buffer to file.",
                    written, size, absoluteFilePath.c_str());
                return false;
            }

            return true;
        }

        bool SaveFileFromBuffer(const AbsolutePath& absoluteFilePath, const Buffer& buffer)
        {
            return SaveFileFromBuffer(absoluteFilePath, buffer.data(), buffer.size());
        }

    } // io
} // base
