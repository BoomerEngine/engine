/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\winapi #]
* [#platform: winapi #]
***/

#include "build.h"
#include "multiQueueWindows.h"

#include <Windows.h>

namespace base
{
    namespace prv
    {

        //--

        WinMultiQueue* WinMultiQueue::Create(uint32_t numQueues)
        {
            return new WinMultiQueue(numQueues);
        }

        //--

        WinMultiQueue::WinMultiQueue(uint32_t numInternalQueues)
            : m_numQueues(numInternalQueues)
            , m_requestExit(false)
            , m_queueCounters(numInternalQueues)
        {
            // setup queue mask, those are the valid bits for all other masks
            ASSERT(numInternalQueues > 0 && numInternalQueues < 31);
            m_validQueueMask = (1U << numInternalQueues) - 1;

            // initialize synchronization stuff
            InitializeCriticalSection(&m_listMutex);

            // create queue semaphores
            m_listCond = new HANDLE[numInternalQueues];
            for (uint32_t i=0; i<numInternalQueues; ++i)
                m_listCond[i] = CreateSemaphore(NULL, 0, INT_MAX, NULL);
        }

        WinMultiQueue::~WinMultiQueue()
        {
            DeleteCriticalSection(&m_listMutex);

            for (uint32_t i = 0; i < m_numQueues; ++i)
                CloseHandle(m_listCond[i]);

            delete[] m_listCond;
        }

        //--

        WinMultiQueue::EntryFreeList::EntryFreeList()
            : m_list(nullptr)
            , m_numFree(0)
            , m_numAllocated(0)
        {}

        WinMultiQueue::Entry* WinMultiQueue::EntryFreeList::alloc()
        {
            //auto lock = CreateLock(lock);

            if (!m_list)
            {
                ++m_numAllocated;
                return new WinMultiQueue::Entry();
            }
            else
            {
                auto freeCount = --m_numFree;
                ASSERT(freeCount >= 0);

                auto ret = m_list;
                m_list = m_list->m_next;
                ret->m_next = nullptr;
                ret->m_prev = nullptr;
                ASSERT(ret->m_payload == nullptr);
                ASSERT(ret->m_queueMask == 0);

                ++m_numAllocated;

                return ret;
            }
        }

        void WinMultiQueue::EntryFreeList::free(WinMultiQueue::Entry* entry)
        {
            //auto lock = CreateLock(lock);

            auto allocatedCount = --m_numAllocated;
            ASSERT(allocatedCount >= 0);

            entry->m_payload = nullptr;
            entry->m_queueMask = 0;
            entry->m_prev = nullptr;

            entry->m_next = m_list;
            m_list = entry;

            ++m_numFree;
        }

        //--

        WinMultiQueue::QueueCounters::QueueCounters(uint32_t numQueues)
            : m_nonZeroMask(0)
            , m_numQueues(numQueues)
        {
            for (uint32_t i=0; i<ARRAY_COUNT(m_counters); ++i)
                m_counters[i] = 0;
        }

        void WinMultiQueue::QueueCounters::add(uint32_t mask)
        {
            auto tempMask = mask;
            for (uint32_t i=0; i<m_numQueues; ++i, tempMask >>= 1)
                if (tempMask & 1)
                    ++m_counters[i];

            m_nonZeroMask.fetch_or(mask);
        }

        void WinMultiQueue::QueueCounters::sub(uint32_t mask)
        {
            uint32_t zeroedMask = 0xFFFFFFFF;
            auto tempMask = mask;
            for (uint32_t i=0; i<m_numQueues; ++i, tempMask >>= 1)
            {
                if (tempMask & 1)
                {
                    if (0 == --m_counters[i])
                    {
                        zeroedMask &= ~(1U << i); // this queue was zeroed
                    }
                }
            }

            m_nonZeroMask.fetch_and(zeroedMask);
        }

        uint32_t WinMultiQueue::QueueCounters::nonZeroMask()
        {
            return m_nonZeroMask.load();
        }

        //--

        WinMultiQueue::QueueState::QueueState()
            : m_head(nullptr)
            , m_tail(nullptr)
            , m_numQueued(0)
        {}

        void WinMultiQueue::QueueState::push(Entry* entry)
        {
            ++m_numQueued;

            if (!m_head)
            {
                entry->m_next = nullptr;
                entry->m_prev = nullptr;
                m_head = entry;
                m_tail = entry;
            }
            else
            {
                entry->m_next = nullptr;
                entry->m_prev = m_tail;
                m_tail->m_next = entry;
                m_tail = entry;
            }
        }

        void WinMultiQueue::QueueState::unlink(Entry* entry)
        {
            auto nextCount = --m_numQueued;
            ASSERT(nextCount >= 0);

            if (entry->m_next)
                entry->m_next->m_prev = entry->m_prev;
            else
                m_tail = entry->m_prev;

            if (entry->m_prev)
                entry->m_prev->m_next = entry->m_next;
            else
                m_head = entry->m_next;

            entry->m_next = nullptr;
            entry->m_prev = nullptr;
        }

        INLINE static uint32_t GetJobPriority(uint32_t mask)
        {
            if (mask & 1) return 0;
            if (mask & 2) return 1;
            if (mask & 4) return 2;
            if (mask & 8) return 3;
            if (mask & 16) return 4;
            if (mask & 32) return 5;
            return 6;
        }

        WinMultiQueue::Entry* WinMultiQueue::QueueState::pop(uint32_t mask)
        {
            WinMultiQueue::Entry* best = nullptr;
            uint32_t bestPriority = 0xFF;

            for (auto cur  = m_head; cur != nullptr && bestPriority != 0; cur = cur->m_next)
            {
                auto prioMask = cur->m_queueMask & mask;
                if (0 != prioMask)
                {
                    auto priority = GetJobPriority(prioMask);
                    if (priority < bestPriority)
                    {
                        bestPriority = priority;
                        best = cur;
                    }
                }
            }

            // use the best
            if (best)
            {
                unlink(best);
                return best;
            }

            // nothing found
            return nullptr;
        }

        void WinMultiQueue::QueueState::inspect(const TQueueInspectorFunc& inspector) const
        {
            for (auto cur  = m_head; cur != nullptr; cur = cur->m_next)
                inspector(cur->m_payload);// , cur->m_queueMask);
        }

        //--

        void WinMultiQueue::close()
        {
            // mark as requesting a close
            m_requestExit = true;

            // unblock all threads
            for (uint32_t i=0; i< m_numQueues; ++i)
                ReleaseSemaphore(m_listCond[i], 32, NULL);

            // TODO: prevent leaks
        }

        void WinMultiQueue::push(void* jobData, uint32_t queueMask)
        {
            ASSERT(queueMask != 0);

            // change the state of the queue
            EnterCriticalSection(&m_listMutex);

            // allocate an entry
            auto entry  = m_entryFreeList.alloc();
            entry->m_payload = jobData;
            entry->m_queueMask = queueMask;

            // put entry in the list
            m_queueList.push(entry);

            // add job to counters for all queues that
            m_queueCounters.add(queueMask);

            // signal waiting threads on queues
            LeaveCriticalSection(&m_listMutex);
            for (uint32_t i = 0; i < m_numQueues; ++i)
            {
                auto thisQueueMask = 1U << i;
                if (0 != (queueMask & thisQueueMask))
                {
                    ReleaseSemaphore(m_listCond[i], 1, NULL);
                }
            }
        }

        void* WinMultiQueue::pop(uint32_t queueMask)
        {
            ASSERT(queueMask != 0);

            // collect semaphores for the masked queues
            DWORD dwNumSeamphores = 0;
            HANDLE hSemaphores[16];
            for (uint32_t i = 0; i < m_numQueues; ++i)
            {
                auto thisQueueMask = 1U << i;
                if (0 != (queueMask & thisQueueMask))
                {
                    hSemaphores[dwNumSeamphores] = m_listCond[i];
                    dwNumSeamphores += 1;
                }
            }

            // we exit this only with valid data or with exit condition
            void* payload = nullptr;
            while (payload == nullptr)
            {
                // wait for some work to be there
                auto ret = WaitForMultipleObjects(dwNumSeamphores, hSemaphores, false, INFINITE);

                // we are kindly requested to exit
                if (m_requestExit)
                    break;

                // get queue index
                if (ret < WAIT_OBJECT_0 || ret >= (WAIT_OBJECT_0 + dwNumSeamphores))
                    continue;

                EnterCriticalSection(&m_listMutex);

                // check if there's still work on that queue
                if (0 == (m_queueCounters.nonZeroMask() & queueMask))
                {
                    LeaveCriticalSection(&m_listMutex);
                    continue;
                }

                // scan for job matching the queue mask
                // TODO: proper priorities ?
                auto entry  = m_queueList.pop(queueMask);
                if (entry != nullptr)
                {
                    ASSERT(0 != (entry->m_queueMask & queueMask));
                    m_queueCounters.sub(entry->m_queueMask);
                    payload = entry->m_payload;
                    m_entryFreeList.free(entry);
                }

                LeaveCriticalSection(&m_listMutex);
            }

            // return the payload
            return payload;
        }

        void WinMultiQueue::inspect(const TQueueInspectorFunc& inspector)
        {
            EnterCriticalSection(&m_listMutex);
            m_queueList.inspect(inspector);
            LeaveCriticalSection(&m_listMutex);
        }

    } // prv
} // base
