/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\winapi #]
* [#platform: windows #]
***/

#include "build.h"
#include "threadWindows.h"
#include <thread>

BEGIN_BOOMER_NAMESPACE()

namespace prv
{

    static void SetThreadName(ThreadID id, const char* name)
    {
        // Name this thread
        struct THREADNAME_INFO
        {
            uint32_t dwType; // must be 0x1000
            LPCSTR szName; // pointer to name (in user addr space)
            uint32_t dwThreadID; // thread ID (-1=caller thread)
            uint32_t dwFlags; // reserved for future use, must be zero
        };

        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name;
        info.dwThreadID = id;
        info.dwFlags = 0;

        __try
        {
            RaiseException(0x406D1388, 0, sizeof(info) / sizeof(uint32_t), (const ULONG_PTR*)&info);
        }
        __except (EXCEPTION_CONTINUE_EXECUTION)
        {
        }
    }

    void WinThread::Close(void* data)
    {
        auto& handle = *(HANDLE*) data;

        // close the thread handle
        if (handle != NULL)
        {
            // hard exit
            auto ret = WaitForSingleObject(handle, 10000);
            if (WAIT_OBJECT_0 != ret)
            {
                TRACE_WARNING("Failed to close thread gracefully (0x{}), so it was terminated without grace.", Hex(ret));

                TerminateThread(handle, 0);
                handle = NULL;
            }

            // release handle
            CloseHandle(handle);
            handle = NULL;
        }
    }

    /*void WinThread::priority(ThreadPriority newPriority)
    {

    }*/

    struct InitPayload
    {
        TThreadFunc func;
        char name[50];
    };

    DWORD __stdcall WinThread::StaticEntry(LPVOID lpThreadParameter)
    {
        auto initData  = (InitPayload*)lpThreadParameter;

        initData->func();

        delete initData;
        return 0;
    }

    //-------------

    void WinThread::Init(void* data, const ThreadSetup& setup)
    {
        auto initData  = new InitPayload;
        initData->func = setup.m_function;
        strcpy_s(initData->name, setup.m_name);

        INT systemPriority = THREAD_PRIORITY_NORMAL;
        switch (setup.m_priority)
        {
        case ThreadPriority::AboveNormal:
            systemPriority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;

        case ThreadPriority::BelowNormal:
            systemPriority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        }

        // Create thread
        DWORD threadID = 0;
        HANDLE handle = CreateThread(NULL, setup.m_stackSize, WinThread::StaticEntry, initData, CREATE_SUSPENDED, &threadID);
        ASSERT_EX(NULL != handle, "Failed to create a thread. Whoops.");

        SetThreadName(threadID, setup.m_name);

        SetThreadPriority(handle, systemPriority);

        if (setup.m_affinity != 0)
            SetThreadIdealProcessor(handle, setup.m_affinity);

        *(HANDLE*)data = handle;
        TRACE_SPAM("Thread '{}' created (ID {}), starting...", setup.m_name, threadID);

        ResumeThread(handle);
    }

} // prv

ThreadID GetCurrentThreadID()
{
    return ::GetCurrentThreadId();
}

uint32_t GetNumberOfCores()
{
    return std::thread::hardware_concurrency();
}

void SetThreadName(const char* name)
{
    prv::SetThreadName(::GetCurrentThreadId(), name);
}

void Sleep(uint32_t ms)
{
    ::Sleep(ms);
}

void Yield()
{
    ::SwitchToThread();
}

END_BOOMER_NAMESPACE()
