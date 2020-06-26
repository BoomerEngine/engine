/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\posix #]
* [#platform: posix #]
***/

#pragma once

#include "absolutePath.h"
#include "base/containers/include/stringBuf.h"

#include "ioFileHandle.h"

namespace base
{
    namespace io
    {
        namespace prv
        {
            class POSIXAsyncReadDispatcher;

            // POSIX based file handle
            class POSIXFileHandle : public IFileHandle
            {
            public:
                POSIXFileHandle(int hFile, const StringBuf& origin, bool reader, bool writer, POSIXAsyncReadDispatcher* dispatcher);
                virtual ~POSIXFileHandle();

                int syncFileHandle() const;

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

                ///--

            protected:
                int m_fileHandle;
                StringBuf m_origin;

                bool m_isReader : 1;
                bool m_isWriter : 1;

                POSIXAsyncReadDispatcher* m_dispatcher;
            };

        } // prv
    } // io
} // base