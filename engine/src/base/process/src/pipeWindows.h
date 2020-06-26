/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\pipe\winapi #]
* [#platform: windows #]
***/

#pragma once

#include "pipe.h"
#include <vector>
#include <string>
#include <atomic>

#include <Windows.h>

namespace base
{
    namespace process
    {
        namespace prv
        {

            /// reader pipe implementation for Windows
            class WinPipeReader : public IPipeReader
            {
            public:
                static WinPipeReader* Create(IOutputCallback* callback);
                static WinPipeReader* Open(const char* pipeName, IOutputCallback* callback);

                //--

                WinPipeReader();
                virtual ~WinPipeReader();

                virtual void close() override final;
                virtual const char* name() const override final;
                virtual bool isOpened() const override final;
                virtual uint32_t read(void* data, uint32_t size) override final;

            protected:
                HANDLE m_hPipe;
                HANDLE m_hReadThread;

                std::atomic<uint32_t> m_overlappedReadCompleted;
                OVERLAPPED m_overlapped;
                IOutputCallback* m_callback;
                std::atomic<uint32_t> m_requestExit;

                char m_name[256];

                std::vector<uint8_t> m_asyncBuffer;

                static DWORD WINAPI ReadPipeProc(LPVOID lpThreadParameter);
                static void WINAPI ProcessOverlappedResult(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
            };

            //-----------------------------------------------------------------------------

            /// writer pipe implementation for Windows
            class WinPipeWriter : public IPipeWriter
            {
            public:
                static WinPipeWriter* Create();
                static WinPipeWriter* Open(const char* pipeName);

                //--

                WinPipeWriter();
                virtual ~WinPipeWriter();

                virtual void close() override final;
                virtual const char* name() const override final;
                virtual bool isOpened() const override final;
                virtual uint32_t write(const void* data, uint32_t size) override final;

            protected:
                HANDLE m_hPipe;
                char m_name[256];
            };

            //-----------------------------------------------------------------------------

        } // prv
    } // process
} // base