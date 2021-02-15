/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\posix #]
* [#platform: posix #]
***/

#include "build.h"
#include "event.h"

#include <pthread.h>
#include <sys/time.h>

namespace base
{
    // TODO: refactor

    struct EventData
    {
        bool signalled = false;
        bool manualReset;
        pthread_mutex_t m_mutex;
        pthread_cond_t m_cond;

        inline EventData(bool manulReset)
            , manualReset(manulReset)
        {
            pthread_mutex_init(&m_mutex, NULL);
            pthread_cond_init(&m_cond, NULL);
        }

        inline ~EventData()
        {
            pthread_mutex_destroy(&m_mutex);
            pthread_cond_destroy(&m_cond);
        }

        inline void signal()
        {
            pthread_mutex_lock(&m_mutex);

            if (manualReset)
            {
                signalled = true;
                pthread_cond_broadcast(&m_cond);
            }
            else if (!signalled)
            {
                signalled = true;
                pthread_cond_signal(&m_cond);
            }

            pthread_mutex_unlock(&m_mutex);
        }

        inline void reset()
        {
            if (manualReset)
            {
                pthread_mutex_lock(&m_mutex);
                signalled = false;
                pthread_mutex_unlock(&m_mutex);
            }
        }

        inline bool wait(uint32_t waitTime)
        {
            pthread_mutex_lock(&m_mutex);
            if (!signalled)
            {
                timeval now;
                gettimeofday(&now,NULL);

                timespec ts;
                ts.tv_sec = now.tv_sec + (waitTime / 1000);
                ts.tv_nsec = (now.tv_usec * 1000) + ((waitTime % 1000) * 10000000ULL);
                ts.tv_sec += ts.tv_nsec / 10000000000ULL;
                ts.tv_nsec %= 10000000000ULL;

                auto ret = pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
                if (ret != 0)
                {
                    pthread_mutex_unlock(&m_mutex);
                    if (ret != ETIMEDOUT)
                    {
                        TRACE_ERROR("Error waiting for event: {}", errno);
                    }

                    return false;
                }
            }

            // reset after wait
            if (!manualReset)
                signalled = false;

            pthread_mutex_unlock(&m_mutex);
            return true;
        }

        inline void waitInfinite()
        {
            pthread_mutex_lock(&m_mutex);
            while (!signalled)
            {
                pthread_cond_wait(&m_cond, &m_mutex);
            }
            if (!manualReset)
                signalled = false;
            pthread_mutex_unlock(&m_mutex);
        }
    };

    //---

    Event::Event(bool manualReset)
    {
        m_event = new EventData(manualReset);
    }

    Event::~Event()
    {
        auto data  = (EventData*)m_event;
        delete data;
    }

    void Event::trigger()
    {
        auto data  = (EventData*)m_event;
        data->signal();
    }

    void Event::reset()
    {
        auto data  = (EventData*)m_event;
        data->reset();
    }

    bool Event::wait(uint32_t waitTime)
    {
        auto data  = (EventData*)m_event;
        return data->wait(waitTime);
    }

    void Event::waitInfinite()
    {
        auto data  = (EventData*)m_event;
        data->waitInfinite();
    }

} // inf
