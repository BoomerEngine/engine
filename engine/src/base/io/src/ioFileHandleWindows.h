/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#pragma once

#include "base/containers/include/stringBuf.h"

#include "ioFileHandle.h"

#include <Windows.h>
#include "ioAsyncFileHandle.h"

namespace base
{
    namespace io
    {
        namespace prv
        {
            ///--

            // WinAPI based file handle for sync READ operations
            class WinReadFileHandle : public IReadFileHandle
            {
            public:
                WinReadFileHandle(HANDLE hSyncFile, const StringView path);
                virtual ~WinReadFileHandle();

                INLINE HANDLE handle() const { return m_hHandle; }

                // IFileHandle implementation
                virtual uint64_t size() const override final;
                virtual uint64_t pos() const override final;
                virtual bool pos(uint64_t newPosition) override final;
                virtual uint64_t readSync(void* data, uint64_t size) override final;

            protected:
                HANDLE m_hHandle; // always there
                StringBuf m_origin;
            };

            ///--

            // WinAPI based file handle for sync WRITE operations
            class WinWriteFileHandle : public IWriteFileHandle
            {
            public:
                WinWriteFileHandle(HANDLE hSyncFile, StringView path);
                virtual ~WinWriteFileHandle();

                INLINE HANDLE handle() const { return m_hHandle; }

                // IFileHandle implementation
                virtual uint64_t size() const override final;
                virtual uint64_t pos() const override final;
                virtual bool pos(uint64_t newPosition) override final;
                virtual uint64_t writeSync(const void* data, uint64_t size) override final;
                virtual void discardContent() override final;

            protected:
                HANDLE m_hHandle; // always there
                StringBuf m_origin;
            };

            ///--

            // WinAPI based file handle for sync WRITE operations
            class WinWriteTempFileHandle : public IWriteFileHandle
            {
            public:
                WinWriteTempFileHandle(Array<wchar_t> targetPath, Array<wchar_t> tempFilePath, const WriteFileHandlePtr& tempFileWriter);
                virtual ~WinWriteTempFileHandle();

                // IFileHandle implementation
                virtual uint64_t size() const override final;
                virtual uint64_t pos() const override final;
                virtual bool pos(uint64_t newPosition) override final;
                virtual uint64_t writeSync(const void* data, uint64_t size) override final;
                virtual void discardContent() override final;

            protected:
                Array<wchar_t> m_tempFilePath;
                Array<wchar_t> m_targetFilePath;
                WriteFileHandlePtr m_tempFileWriter;
            };

            //--

            class WinAsyncReadDispatcher;

            // WinAPI based file handle
            class WinAsyncFileHandle : public IAsyncFileHandle
            {
            public:
                WinAsyncFileHandle(HANDLE hAsyncFile, StringView origin, uint64_t size, WinAsyncReadDispatcher* dispatcher);
                virtual ~WinAsyncFileHandle();

                INLINE HANDLE handle() const { return m_hHandle; }

                // IFileHandle implementation
                virtual uint64_t size() const override final;

                virtual CAN_YIELD uint64_t readAsync(uint64_t offset, uint64_t size, void* readBuffer) override final;

            protected:
                HANDLE m_hHandle; // always there
                uint64_t m_size; // at the time file was opened

                WinAsyncReadDispatcher* m_dispatcher;

                StringBuf m_origin;
            };


        } // prv
    } // io
} // base