/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: impl #]
***/

#include "build.h"
#include "base/system/include/scopeLock.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/system/include/thread.h"
#include "fiberSystemCommon.h"

#ifdef PLATFORM_GCC
    #pragma GCC push_options
    #pragma GCC optimize ("O0")
#else
    #pragma optimize("",off)
#endif

namespace base
{
    namespace fibers
    {
        namespace prv
        {

            ///---

            BaseScheduler::WaitCounterPool::WaitCounterPool()
                : m_seqId(100)
                , m_waitLists(nullptr)
                , m_freeWaitLists(nullptr)
                , m_activeWaitLists(nullptr)
                , m_numActiveWaitLists(0)
            {}

            BaseScheduler::WaitCounterPool::~WaitCounterPool()
            {}

            void BaseScheduler::WaitCounterPool::init(uint32_t maxWaitLists)
            {
                m_maxCounterId = maxWaitLists;
                m_waitLists = new WaitList[maxWaitLists];
                m_seqId = 100;

                m_freeWaitLists = nullptr;
                for (uint32_t i = 0; i < maxWaitLists; ++i)
                {
                    m_waitLists[i].localId = (WaitCounterID)i;
                    m_waitLists[i].listNext = m_freeWaitLists;
                    m_freeWaitLists = &m_waitLists[i];
                }
            }

            void BaseScheduler::WaitCounterPool::release_NoLock(WaitList *entry)
            {
                // remove from active list
                if (entry->listNext)
                    entry->listNext->listPrev = entry->listPrev;
                if (entry->listPrev)
                    entry->listPrev->listNext = entry->listNext;
                else
                    m_activeWaitLists = entry->listNext;
                auto newCount = --m_numActiveWaitLists;
                ASSERT(newCount >= 0);

                // add to free list
                entry->listPrev = nullptr;
                entry->listNext = m_freeWaitLists;
                m_freeWaitLists = entry;
            }

            BaseScheduler::WaitList* BaseScheduler::WaitCounterPool::alloc_NoLock()
            {
                // get from list
                ASSERT(m_freeWaitLists);
                auto entry  = m_freeWaitLists;
                m_freeWaitLists = entry->listNext;
                ASSERT(!entry->listPrev);
                entry->listNext = nullptr;

                // lint to active list
                if (m_activeWaitLists)
                    m_activeWaitLists->listPrev = entry;
                entry->listNext = m_activeWaitLists;
                m_activeWaitLists = entry;
                ++m_numActiveWaitLists;

                return entry;
            }

            WaitCounter BaseScheduler::WaitCounterPool::allocWaitCounter(const char* userName, uint32_t initialCount)
            {
                auto lock = base::CreateLock(m_lock);

                // alloc
                auto counter  = alloc_NoLock();

                // setup
                ASSERT(counter->seqId == 0);
                ASSERT(counter->currentCount.load() == 0);
                ASSERT(counter->jobs == nullptr);
                ASSERT(counter->userName == nullptr);
                counter->currentCount = initialCount;
                counter->seqId = ++m_seqId;
                counter->userName = userName;

                // return handle
                return WaitCounter(counter->localId, counter->seqId);
            }

            void BaseScheduler::WaitCounterPool::releaseWaitCounter(const WaitCounter& counter)
            {
                auto lock = base::CreateLock(m_lock);

                // check ID
                auto counterId = counter.id();
                ASSERT(counterId < m_maxCounterId);

                // validate entry
                auto& entry = m_waitLists[counterId];
                ASSERT_EX(entry.seqId == counter.sequenceId(), "Trying to release a counter that was already released");
                ASSERT_EX(entry.jobs == nullptr, "Trying to free wait counter that has jobs waiting for it");
                ASSERT_EX(entry.currentCount.load() == 0, "Trying to free wait counter that has jobs waiting for it");

                // cleanup
                entry.seqId = 0;
                entry.userName = nullptr;
                entry.jobs = nullptr;
                entry.currentCount = 0;

                // free
                release_NoLock(&entry);
            }

            void BaseScheduler::WaitCounterPool::inspect(const std::function<void(const WaitList* list)>& inspector)
            {
                auto lock = CreateLock(m_lock);

                for (auto cur  = m_activeWaitLists; cur; cur = cur->listNext)
                    inspector(cur);
            }

            void BaseScheduler::WaitCounterPool::validate()
            {
                auto lock = CreateLock(m_lock);

                uint32_t count = 0;
                for (auto cur  = m_activeWaitLists; cur; cur = cur->listNext)
                {
                    ASSERT(cur->currentCount.load() > 0);
                    ASSERT(cur->seqId != 0);
                    count += 1;
                }
                ASSERT(count == m_numActiveWaitLists.load());

                for (auto cur  = m_freeWaitLists; cur; cur = cur->listNext)
                {
                    ASSERT(cur->currentCount.load() == 0);
                    ASSERT(cur->seqId == 0);
                    ASSERT(cur->jobs == nullptr);
                }
            }

            bool BaseScheduler::WaitCounterPool::checkWaitCounter(const WaitCounter& counter)
            {
                auto lock = CreateLock(m_lock);

                // get counter info
                auto counterId = counter.id();
                ASSERT(counter.sequenceId() != 0);
                ASSERT(counterId < m_maxCounterId);
                auto& entry = m_waitLists[counterId];
                if (entry.seqId != counter.sequenceId())
                    return true; // counter already released

                // counter is in use
                return false;
            }

            bool BaseScheduler::WaitCounterPool::addJobToWaitingList(const WaitCounter& counter, PendingJob* job)
            {
                // job entering this function must be in the waiting state
                ASSERT(job->state.load() == PendingJobState::Waiting);
                ASSERT(job->waitList == nullptr);

                // we cannot wait on the zero counter
                if (counter.empty())
                    return false;

                auto lock = base::CreateLock(m_lock);

                // check if the counter is valid
                auto counterId = counter.id();
                ASSERT(counter.sequenceId() != 0);
                ASSERT(counterId < m_maxCounterId);
                auto& entry = m_waitLists[counterId];
                if (entry.seqId != counter.sequenceId())
                    return false; // counter already released

                // create a wait entry
                job->waitList = &entry;
                //debug::GrabCallstack(1, nullptr, job->waitCallstack);

                // job cannot be in pending list
                ASSERT(job->next == nullptr);
                job->next = entry.jobs;
                entry.jobs = job;

                // we have entered waiting state, schedule back to the idle fiber
                return true;
            }

            void BaseScheduler::WaitCounterPool::printCounterInfo(const WaitList& entry)
            {
                if (entry.userName != nullptr && *entry.userName)
                    fprintf(stderr, "'%s' ", entry.userName);

                fprintf(stderr, "VAL %u ", entry.currentCount.load());

                // list jobs
                for (auto waitList  = entry.jobs; waitList != nullptr; waitList = waitList->next)
                    fprintf(stderr, "-> JOB %u (%s) ", waitList->jobId, waitList->job.name);
            }

            void BaseScheduler::WaitCounterPool::printCounterInfo(const WaitCounter& counter)
            {
                auto lock = base::CreateLock(m_lock);

                if (counter.empty())
                {
                    fprintf(stderr, "EMPTY");
                    return;
                }

                // check if the counter is valid
                auto counterId = counter.id();
                ASSERT(counter.sequenceId() != 0);
                ASSERT(counterId < m_maxCounterId);

                fprintf(stderr, "CID%u SID%u ", counter.id(), counter.sequenceId());;

                auto& entry = m_waitLists[counterId];
                if (entry.seqId != counter.sequenceId())
                {
                    fprintf(stderr, "RELEASED");
                    return;
                }

                printCounterInfo(entry);
            }

            WaitCounter BaseScheduler::WaitCounterPool::findCounter(uint32_t id)
            {
                if (id >= m_maxCounterId)
                    return WaitCounter();

                auto& entry = m_waitLists[id];
                if (entry.currentCount.load() == 0)
                    return WaitCounter();

                return WaitCounter(entry.localId, entry.seqId);
            }

            BaseScheduler::PendingJob* BaseScheduler::WaitCounterPool::signalWaitCounter(const WaitCounter& counter, uint32_t count)
            {
                auto lock = base::CreateLock(m_lock);

                // empty counter
                if (counter.empty())
                    return nullptr;

                // get counter state
                auto counterId = counter.id();
                ASSERT(counterId < m_maxCounterId);
                auto& entry = m_waitLists[counterId];
                ASSERT_EX(entry.seqId == counter.sequenceId(), "Trying to signal counter that was freed");
                if (entry.seqId != counter.sequenceId())
                    return nullptr;

                // decrement the counter value, this does not require a lock on the whole list
                auto newValue = entry.currentCount -= count;
                ASSERT(newValue >= 0);

                // if the counter value is not yet a zero we wont unlock any jobs
                if (newValue > 0)
                    return nullptr;

                // get the list of jobs from the wait list
                auto jobList  = entry.jobs;
                entry.jobs = nullptr;

                // release the wait list
                releaseWaitCounter(counter);

                // return extracted job list
                return jobList;
            }

            ///---

        } // prv
    } // fibers
} // base


#ifdef PLATFORM_GCC
    #pragma GCC pop_options
#else
    #pragma optimize("",on)
#endif