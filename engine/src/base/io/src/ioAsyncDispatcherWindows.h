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
#include "base/containers/include/queue.h"
#include "base/system/include/thread.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/memory/include/structurePool.h"

#include <Windows.h>

namespace base
{
    namespace io
    {
        namespace prv
        {

            class WinFileHandle;

            // dispatch for IO jobs
            class WinAsyncReadDispatcher : public base::NoCopy
            {
            public:
                WinAsyncReadDispatcher(uint32_t maxInFlightRequests);
                ~WinAsyncReadDispatcher();

                // process async IO request, returns the number of bytes read
                CAN_YIELD uint64_t readAsync(HANDLE hSyncFile, HANDLE hAsyncFile, uint64_t offset, uint64_t size, void* outMemory);

            private:
                struct Token
                {
                    OVERLAPPED m_overlapped; // MUST BE FIRST
                    void* m_memory;
                    HANDLE m_hSyncHandle;
                    HANDLE m_hAsyncHandle;
                    uint32_t m_size;
                    fibers::WaitCounter m_signal;
                    uint32_t* m_numBytesRead;
                    WinAsyncReadDispatcher* m_dispatcher;

                    INLINE Token()
                        : m_memory(nullptr)
                        , m_hSyncHandle(NULL)
                        , m_hAsyncHandle(NULL)
                        , m_numBytesRead(nullptr)
                        , m_size(0)
                        , m_dispatcher(nullptr)
                    {
                        memzero(&m_overlapped, sizeof(m_overlapped));
                    }
                };

                mem::StructurePool<Token> m_tokenPool;
				SpinLock m_tokenPoolLock;

                Thread m_ioCompletionThread;

                Queue<Token*> m_tokensToExecute;
                SpinLock m_tokensToExecuteLock;

                Semaphore m_tokensCounter;
                std::atomic<uint32_t> m_exiting;

                Token* popTokenFromQueue();
                Token* allocToken();
				void releaseToken(Token* token);

                void threadFunc();

                static void WINAPI ProcessOverlappedResult(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
            };

        } // prv
    } // io
} // base