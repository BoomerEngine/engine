/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: public #]
***/

#include "build.h"
#include "fiberSyncPoint.h"

namespace base
{
    namespace fibers
    {

        ///--

        SyncPoint::SyncPoint(const char* name)
            : m_completed(true)
            , m_name(name)
        {}

        SyncPoint::~SyncPoint()
        {}

        CAN_YIELD void SyncPoint::acquire()
        {
            auto lock = CreateLock(m_lock);

            // check if we have not yet completed the sync (no release was called)
            if (!m_completed)
            {
                // create the fence we will be waiting for
                ASSERT_EX(m_fence.empty(), "Fence already exists");
                auto createdFence = Fibers::GetInstance().createCounter(m_name, 1);
                m_fence = createdFence;

                // release our data lock, this will allow other tread to see the data
                lock.release();

                // wait for the fence
                // NOTE: we cannot use the m_fence since we are outside the lock
                Fibers::GetInstance().waitForCounterAndRelease(createdFence);

                // reacquire the fence
                lock.aquire();

                // we should be completed by now
                ASSERT_EX(m_completed, "Sync point fence passed by flag not set");
                ASSERT_EX(m_fence.empty(), "Sync point fence not consumed");
            }

            // mark as incomplete so the release can work
            m_completed = false;
        }

        void SyncPoint::release()
        {
            auto lock = CreateLock(m_lock);

            // mark as completed, if we managed to get to release() before next acquire() no fence will be created
            ASSERT_EX(m_completed == false, "Release without previous acquire");
            m_completed = true;

            // do we want to wait for a fence ?
            if (!m_fence.empty())
            {
                // consume the fence
                auto fenceToWait = m_fence;
                m_fence = WaitCounter();
                Fibers::GetInstance().signalCounter(fenceToWait);
            }
        }

        ///--

    } // fibers
} // base
