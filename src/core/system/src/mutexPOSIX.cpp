/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "mutex.h"

#include <pthread.h>
#include <syscall.h>

namespace base
{
    Mutex::Mutex()
    {
        static_assert(sizeof(Mutex::m_data) >= sizeof(pthread_mutex_t), "Critical section data to small");
        memset(&m_data, 0, sizeof(m_data));

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init((pthread_mutex_t*)&m_data, &attr);
        pthread_mutexattr_destroy(&attr);

        pthread_mutex_lock((pthread_mutex_t*)&m_data);
        pthread_mutex_unlock((pthread_mutex_t*)&m_data);

        auto owningThread  = (uint32_t*)&m_data[__SIZEOF_PTHREAD_MUTEX_T];
        *owningThread = 0;
    }

    Mutex::~Mutex()
    {
        auto owningThread  = (std::atomic<uint32_t>*)&m_data[__SIZEOF_PTHREAD_MUTEX_T];
        ASSERT(owningThread->exchange(0) == 0);

        pthread_mutex_destroy((pthread_mutex_t*)&m_data);
    }

    void Mutex::acquire()
    {
        pthread_mutex_lock((pthread_mutex_t*)&m_data);

        /*auto owningThread  = (std::atomic<uint32_t>*)&m_data[__SIZEOF_PTHREAD_MUTEX_T];
        auto tid = syscall(SYS_gettid);
        auto prevTid = owningThread->exchange(tid);
        ASSERT(0 == prevTid || tid == prevTid);*/
    }

    void Mutex::release()
    {
        auto tid = syscall(SYS_gettid);

        auto mutexData = (pthread_mutex_t *) &m_data;
        ASSERT_EX(tid == mutexData->__data.__owner || 0 == mutexData->__data.__owner, "Mutex must be released only by the thread that acquired it");

        /*auto owningThread  = (std::atomic<uint32_t>*)&m_data[__SIZEOF_PTHREAD_MUTEX_T];
        auto lockCount  =  (std::atomic<uint32_t>*)&m_data[__SIZEOF_PTHREAD_MUTEX_T + 4];
        if (0 == --(*lockCount))
        {
            auto owningTid = owningThread->exchange(0);
            ASSERT(owningTid == tid);
        }*/

        pthread_mutex_unlock((pthread_mutex_t *) &m_data);
    }

    void Mutex::spinCount(uint32_t spinCount)
    {
    }

} // base
