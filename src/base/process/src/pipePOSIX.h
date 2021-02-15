/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\pipe\posix #]
* [#platform: posix #]
***/

#pragma once

#include "pipe.h"
#include <vector>
#include <string>
#include <atomic>

namespace base
{
    namespace process
    {
        namespace prv
        {

            //-----------------------------------------------------------------------------

            /// POSIX based pipe writer
            class POSIXPipeWriter : public IPipeWriter
            {
            public:
                static POSIXPipeWriter* Create();
                static POSIXPipeWriter* Open(const char* pipeName);

                //--

                POSIXPipeWriter();
                virtual ~POSIXPipeWriter();

                virtual void close() override final;
                virtual const char* name() const override final;
                virtual bool isOpened() const override final;
                virtual uint32_t write(const void* data, uint32_t size) override final;

            protected:
                int m_handle;
                char m_name[128];
            };

            //-----------------------------------------------------------------------------

            /// POSIX based pipe reader
            class POSIXPipeReader : public IPipeReader
            {
            public:
                static POSIXPipeReader* Open(const char* pipeName, IOutputCallback* callback);
                static POSIXPipeReader* Create(IOutputCallback* callback);

                static POSIXPipeReader* OpenNative(const char* fullName, IOutputCallback* callback);

                //--

                POSIXPipeReader();
                virtual ~POSIXPipeReader();

                virtual void close() override final;
                virtual const char* name() const override final;
                virtual bool isOpened() const override final;
                virtual uint32_t read(void* data, uint32_t size) override final;

            protected:
                int m_handle;
                char m_name[128];
                pthread_t m_readThread;
                volatile bool m_requestExit;
                IOutputCallback* m_callback;

                static void* ThreadFunc(void* param);
            };

        } // prv
    } // process
} // base