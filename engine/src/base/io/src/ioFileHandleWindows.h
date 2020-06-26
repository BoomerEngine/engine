/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#pragma once

#include "absolutePath.h"
#include "base/containers/include/stringBuf.h"

#include "ioFileHandle.h"

#include <Windows.h>

namespace base
{
    namespace io
    {
        namespace prv
        {
            class WinAsyncReadDispatcher;

            // WinAPI based file handle
            class WinFileHandle : public IFileHandle
            {
            public:
                WinFileHandle(HANDLE hSyncFile, HANDLE hAsyncFile, const StringBuf& origin, bool reader, bool writer, bool locked, WinAsyncReadDispatcher* dispatcher);
                virtual ~WinFileHandle();

                HANDLE syncFileHandle() const;
                HANDLE asyncFileHandle() const;

                // IFileHandle implementation
                virtual const StringBuf& originInfo() const override final;
                virtual uint64_t size() const override final;
                virtual uint64_t pos() const override final;
                virtual bool pos(uint64_t newPosition) override final;

                virtual bool isReadingAllowed() const override final;
                virtual bool isWritingAllowed() const override final;

                virtual uint64_t writeSync(const void* data, uint64_t size) override final;
                virtual uint64_t readSync(void* data, uint64_t size) override final;

                virtual CAN_YIELD uint64_t readAsync(uint64_t offset, uint64_t size, void* readBuffer) override final;

            protected:
                HANDLE m_hSyncFile; // always there

                mutable HANDLE m_hAsyncFile; // not created until requested
                mutable CRITICAL_SECTION m_asyncHandleLock; // prevents race on the m_hAsyncFile

                StringBuf m_origin;

                bool m_isReader : 1;
                bool m_isWriter : 1;
                bool m_isLocked : 1;

                WinAsyncReadDispatcher* m_dispatcher;
            };

        } // prv
    } // io
} // base