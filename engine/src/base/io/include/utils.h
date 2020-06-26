/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

namespace base
{
    class StrintBuf;

    namespace io
    {
        class IFileHandle;
        class AbsolutePath;

        // load content of an absolute file on disk to a string buffer
        // returns true if content was loaded, false if there were errors (file does not exist, etc)
        // NOTE: both ANSI and UTF-16 files are supported
        extern BASE_IO_API bool LoadFileToString(const AbsolutePath& absoluteFilePath, StringBuf &outString);

        // load content of an absolute file on disk to an array
        // returns true if content was loaded, false if there were errors (file does not exist, etc)
        // NOTE: both ANSI and UTF-16 files are supported
        extern BASE_IO_API bool LoadFileToArray(const AbsolutePath& absoluteFilePath, Array<uint8_t>& outArray);

        // load content of an absolute file on disk as a buffer in memory allocated via MemAlloc
        // returns true if content was loaded, false if there were errors (file does not exist, etc)
        extern BASE_IO_API bool LoadFileToBuffer(const AbsolutePath& absoluteFilePath, void* &outBuffer, uint64_t& outBufferSize);

        // load content of an absolute file on disk as a buffer in memory allocated via MemAlloc
        // returns the buffer handle or nullptr if there were errors (file does not exist, etc)
        extern BASE_IO_API Buffer LoadFileToBuffer(const AbsolutePath& absoluteFilePath);

        //----

        // save string (ANSI / UTF16) to file on disk
        // returns true if content was saved, false if there were errors (file could not be created, etc)
        extern BASE_IO_API bool SaveFileFromString(const AbsolutePath& absoluteFilePath, StringView<char>str, bool append = false);

        // save block of memory to file on disk
        // returns true if content was saved, false if there were errors (file could not be created, etc)
        extern BASE_IO_API bool SaveFileFromBuffer(const AbsolutePath& absoluteFilePath, const void* buffer, size_t size, bool append = false);

        // save block of memory to file on disk
        // returns true if content was saved, false if there were errors (file could not be created, etc)
        extern BASE_IO_API bool SaveFileFromBuffer(const AbsolutePath& absoluteFilePath, const Buffer& buffer, bool append = false);

        //---

        // copy content of one physical file to other physical file
        // returns true if content was copied, false if there were errors
        extern BASE_IO_API bool CopyContent(IFileHandle& reader, IFileHandle& writer, size_t sizeClamp = MAX_SIZE_T);

        // load content of file into buffer allocated via MemAlloc
        // returns the buffer handle
        extern BASE_IO_API Buffer LoadContentToBuffer(IFileHandle& reader, uint64_t initialOffset = 0, size_t sizeClamp = MAX_SIZE_T);

    } // io
} // base