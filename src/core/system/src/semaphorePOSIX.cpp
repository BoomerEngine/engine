/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "semaphoreCounter.h"

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

namespace base
{
    // TODO: refactor

    struct SemaphoreData
    {
        sem_t data;

        inline SemaphoreData(uint32_t initialCount)
        {
            memset(&data, 0, sizeof(data));
            sem_init(&data, 0, initialCount);
        }

        inline ~SemaphoreData()
        {
            sem_destroy(&data);
        }

        inline void signal()
        {
            sem_post(&data);
        }

        inline bool wait(uint32_t timeout)
        {
            if (timeout == INFINITE_TIME)
            {
                sem_wait(&data);
                return true;
            }
            else
            {
                timeval now;
                gettimeofday(&now,NULL);

                timespec ts;
                ts.tv_sec = now.tv_sec + (timeout / 1000);
                ts.tv_nsec = (now.tv_usec * 1000) + ((timeout % 1000) * 10000000ULL);
                ts.tv_sec += ts.tv_nsec / 10000000000ULL;
                ts.tv_nsec %= 10000000000ULL;

                if (-1 == sem_timedwait(&data, &ts))
                {
                    if (errno == ETIMEDOUT)
                        return false;

                    TRACE_ERROR("Semaphore error: {}", errno);
                    return false;
                }

                return true;
            }
        }
    };

    //---

    //--

    Semaphore::Semaphore(uint32_t initialCount, uint32_t maxCount)
    {
        m_handle = new SemaphoreData(initialCount);
    }

    Semaphore::~Semaphore()
    {
        auto data  = (SemaphoreData*)m_handle;
        delete data;
    }

    uint32_t Semaphore::release(uint32_t count)
    {
        auto data  = (SemaphoreData*)m_handle;
        data->signal();
        return 0;
    }

    bool Semaphore::wait(uint32_t waitTime)
    {
        auto data  = (SemaphoreData*)m_handle;
        return data->wait(waitTime);
    }

    int Semaphore::WaitMultiple(Semaphore** semaphores, uint32_t numSemaphores, uint32_t waitTime /*= INFINITE_TIME*/)
    {
        if (numSemaphores == 0)
            return 0;

        // TODO! fuck
        if (!semaphores[0]->wait(waitTime))
            return -1;
        return 0;
    }

} // base
