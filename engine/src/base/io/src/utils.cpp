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

            return LoadContentToBuffer(*file, 0, MAX_SIZE_T);
        }

        bool LoadFileToBuffer(const AbsolutePath& absoluteFilePath, void* &outBuffer, uint64_t& outBufferSize)
        {
            // load buffer
            auto file  = IO::GetInstance().openForReading(absoluteFilePath);
            if (!file)
                return false;

            // prepare buffer
            auto size  = file->size();
            if (size >= (uint64_t)MAX_IO_SIZE)
            {
                TRACE_ERROR("File '{}' is bigger than the largest file allowed to be read by application. Consider switching to 64-bit builds.", absoluteFilePath);
                return false;
            }

            // allocate memory
            auto loadSize  = range_cast<size_t>(size);
            auto mem  = MemAlloc(POOL_IO, loadSize, 1);
            if (!mem)
            {
                TRACE_ERROR("Failed to allocate buffer for reading content of file '{}', neede {} llu bytes", absoluteFilePath, size);
                return false;
            }

            // load file data
            auto readSize  = file->readSync(outBuffer, loadSize);
            if (readSize != loadSize)
            {
                TRACE_ERROR("Read {} bytes instead of {} from '{}'. Failed to load file to buffer.",
                    readSize, loadSize, absoluteFilePath.c_str());

                MemFree(outBuffer);
                outBuffer = nullptr;
                outBufferSize = 0;
                return false;
            }

            // no errors
            outBuffer = mem;
            outBufferSize = size;
            return true;
        }

        bool SaveFileFromString(const AbsolutePath& absoluteFilePath, StringView<char> str, bool append /*= false*/)
        {
            // open file
            auto file  = IO::GetInstance().openForWriting(absoluteFilePath, append);
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

        bool SaveFileFromBuffer(const AbsolutePath& absoluteFilePath, const void* buffer, size_t size, bool append /*=false*/)
        {
            // open file
            auto file  = IO::GetInstance().openForWriting(absoluteFilePath, append);
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

        bool SaveFileFromBuffer(const AbsolutePath& absoluteFilePath, const Buffer& buffer, bool append /*= false*/)
        {
            return SaveFileFromBuffer(absoluteFilePath, buffer.data(), buffer.size(), append);
        }

        bool CopyContent(IFileHandle& reader, IFileHandle& writer, size_t sizeClamp)
        {
            uint8_t buffer[65536];

            uint64_t sizeLeft = std::min<uint64_t>(sizeClamp, reader.size() - reader.pos());
            while (sizeLeft > 0)
            {
                auto sizeToRead  = (uint32_t)std::min<uint64_t>(sizeof(buffer), sizeLeft);

                // load existing content
                auto read  = reader.readSync(buffer, sizeToRead);
                if (read != sizeToRead)
                {
                    TRACE_ERROR("Read {} bytes instead of {} when copying from file '{}'.", read, sizeToRead, writer.originInfo());
                    return false;
                }

                // save to target
                auto written  = writer.writeSync(buffer, sizeToRead);
                if (written != sizeToRead)
                {
                    TRACE_ERROR("Written {} bytes instead of {} when copying to file '{}'.", written, sizeToRead, writer.originInfo());
                    return false;
                }

                sizeLeft -= sizeToRead;
            }

            return true;
        }

        Buffer LoadContentToBuffer(IFileHandle& reader, uint64_t initialOffset /*= 0*/, size_t sizeClamp /*= INDEX_MAX64*/)
        {
            // reading not allowed
            if (!reader.isReadingAllowed())
            {
                TRACE_ERROR("File '{}' does not allow reading", reader.originInfo());
                return nullptr;
            }

            // check if offset is within the file
            if (initialOffset > reader.size())
            {
                TRACE_ERROR("Initial postion {} for file '%hls' is outside the file", initialOffset, reader.originInfo());
                return Buffer();
            }

            // go to position in file
            reader.pos(initialOffset);

            // calculate size to load
            auto loadSize  = std::min<uint64_t>(sizeClamp, reader.size() - initialOffset);
            if (loadSize >= MAX_IO_SIZE)
            {
                TRACE_ERROR("Trying to read more from the file '{}' than the biggets allowed size, consider switching to 64-bit", reader.originInfo());
                return nullptr;
            }

            // allocate buffer
            auto buffer = Buffer::Create(POOL_MEM_BUFFER, loadSize);
            if (!buffer)
            {
                TRACE_ERROR("Unable to allocate {} bytes of memory for reading content of file '{}'", loadSize, reader.originInfo());
                return nullptr;
            }

            // load content
            auto actuallyLoadedSize  = reader.readSync(buffer.data(), loadSize);
            if (actuallyLoadedSize != loadSize)
            {
                TRACE_ERROR("Unable to read {} bytes at {} from file '{}'", loadSize, initialOffset, reader.originInfo());
                return nullptr;
            }

            // yay, data read
            return buffer;
        }

    } // io
} // base
