/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system\impl #]
* [#platform: winapi #]
***/

#include "build.h"
#include "ioFileHandleWindows.h"
#include "ioAsyncDispatcherWindows.h"

BEGIN_BOOMER_NAMESPACE_EX(io::prv)

WinAsyncReadDispatcher::WinAsyncReadDispatcher(uint32_t maxInFlightRequests)
    : m_tokensCounter(0, maxInFlightRequests)
    , m_tokenPool(POOL_IO, maxInFlightRequests)
    , m_exiting(0)
{
    // create the io completion thread
    ThreadSetup setup;
    setup.m_function = [this]() { threadFunc(); };
    setup.m_priority = ThreadPriority::AboveNormal;
    m_ioCompletionThread.init(setup);
}

WinAsyncReadDispatcher::~WinAsyncReadDispatcher()
{
    m_exiting = 1;
    m_tokensCounter.release(1000);
    m_ioCompletionThread.close();
}

WinAsyncReadDispatcher::Token* WinAsyncReadDispatcher::allocToken()
{
	auto lock  = CreateLock(m_tokenPoolLock);
	return m_tokenPool.create();
}

void WinAsyncReadDispatcher::releaseToken(Token* token)
{
	auto lock  = CreateLock(m_tokenPoolLock);
	m_tokenPool.free(token);
}

uint64_t WinAsyncReadDispatcher::readAsync(HANDLE hAsyncFile, uint64_t offset, uint64_t size, void* outMemory)
{
    ASSERT_EX(hAsyncFile != INVALID_HANDLE_VALUE, "Invalid file handle");

    // nothing to read
    if (!size)
        return 0;

    // create request, if this fails it means we are already full, do idle spin until we are free again
    auto token  = allocToken();

    uint32_t numBytesRead = 0;
    auto signal  = Fibers::GetInstance().createCounter("IOCompletedSignal");

    // setup 
    token->m_hAsyncHandle = hAsyncFile;
    token->m_memory = outMemory;
    token->m_signal = signal;
    token->m_size = (uint32_t)size; // TODO: async reads of more than 4GB!
    token->m_overlapped.Offset = (uint32_t)(offset >> 0);
    token->m_overlapped.OffsetHigh = (uint32_t)(offset >> 32);
    token->m_numBytesRead = &numBytesRead;
    token->m_dispatcher = this;

    // send to the thread
    {
        auto lock  = CreateLock(m_tokensToExecuteLock);
        m_tokensToExecute.push(token);
    }
    m_tokensCounter.release(1);

    // wait for the signal from IO thread
    Fibers::GetInstance().waitForCounterAndRelease(signal);
    return numBytesRead;
}

void WINAPI WinAsyncReadDispatcher::ProcessOverlappedResult(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    auto token  = (Token*)lpOverlapped;

    // IO ERROR ?
    if (dwErrorCode != 0)
    {
        TRACE_ERROR("AsyncRead failed with %08X", dwErrorCode);
        *token->m_numBytesRead = 0;
    }
    else
    {
        // write number of bytes that we have written
        *token->m_numBytesRead = range_cast<uint32_t>(dwNumberOfBytesTransfered);
    }

    // release token to pool
    auto signal  = token->m_signal;
    auto dispatcher  = token->m_dispatcher;
    dispatcher->releaseToken(token);

    // signal to unblock the job
    Fibers::GetInstance().signalCounter(signal);
}

WinAsyncReadDispatcher::Token* WinAsyncReadDispatcher::popTokenFromQueue()
{
    auto lock  = CreateLock(m_tokensToExecuteLock);
    if (m_tokensToExecute.empty())
        return nullptr;

    auto token  = m_tokensToExecute.top();
    m_tokensToExecute.pop();
    return token;
}

void WinAsyncReadDispatcher::threadFunc()
{
    for (;;)
    {
        // wait for work
        m_tokensCounter.wait();

        // end of work
        if (m_exiting.load())
            break;

        if (auto token  = popTokenFromQueue())
        {
            ASSERT(token != nullptr);

            // this function must be called from this thread because only here we are waiting in the alert alertable
            if (!ReadFileEx(token->m_hAsyncHandle, token->m_memory, token->m_size, &token->m_overlapped, &ProcessOverlappedResult))
            {
                auto errorCode  = GetLastError();
                TRACE_ERROR("AsyncRead failed with {}", errorCode);

                // return error state
                *token->m_numBytesRead = 0;
                Fibers::GetInstance().signalCounter(token->m_signal);

                // release token here since the callback will not be called
                m_tokenPool.free(token);
            }
        }
    }
}

END_BOOMER_NAMESPACE_EX(io::prv)
