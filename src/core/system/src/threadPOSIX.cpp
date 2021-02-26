/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "threadPOSIX.h"

#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <thread>

namespace base
{
    namespace prv
    {

        void POSIXThread::Close(void* data)
        {
            auto ptr  = (POSIXThread*) data;

            // wait for the thread to finish
            pthread_join(ptr->m_handle, NULL);
            ptr->m_handle = 0;
        }

        struct InitPayload
        {
            TThreadFunc func;
            char name[50];
        };

        void* POSIXThread::ThreadFunc(void *ptr)
        {
            auto initData  = (InitPayload*)ptr;

            pthread_setname_np(pthread_self(), initData->name);

            initData->func();
            delete initData;

            pthread_exit(0);
            return 0;
        }

        void POSIXThread::Init(void* data, const ThreadSetup& setup)
        {
            auto ptr  = (POSIXThread*) data;

            auto initData  = new InitPayload();
            initData->func = setup.m_function;
            strcpy_s(initData->name, setup.m_name);

            pthread_attr_t attrs;
            pthread_attr_init(&attrs);
            pthread_attr_setstacksize(&attrs, setup.m_stackSize);
            pthread_create(&ptr->m_handle, &attrs, &POSIXThread::ThreadFunc, initData);
            pthread_attr_destroy(&attrs);

            ASSERT(ptr->m_handle);
        }

    } // prv

    //--

    ThreadID GetCurrentThreadID()
    {
        return syscall(SYS_gettid);
    }

    uint32_t GetNumberOfCores()
    {
        return std::thread::hardware_concurrency();
    }

    void SetThreadName(const char* name)
    {
        pthread_setname_np(pthread_self(), name);
    }

    void Sleep(uint32_t ms)
    {
        usleep(ms * 1000);
    }

    void Yield()
    {
        sched_yield();
    }

} // base
