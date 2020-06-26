/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\posix #]
* [#platform: posix #]
***/

#pragma once

#include "thread.h"
#include "spinLock.h"
#include "multiQueue.h"
#include "atomic.h"

namespace base
{
    namespace prv
    {

        /// pthreads based queue
        class MultiPOSIXQueue : public IMultiQueue
        {
        public:
            static MultiPOSIXQueue* Create(uint32_t numQueues);

        private:
            MultiPOSIXQueue(uint32_t numInternalQueues);
            virtual ~MultiPOSIXQueue();

            virtual void close() override final;
            virtual void push(void* jobData, uint32_t queueMask) override final;
            virtual void* pop(uint32_t queueMask) override final;
            virtual void inspect(const TQueueInspectorFunc& inspectorFunc) override final;

            //---

            uint32_t m_numQueues;
            uint32_t m_validQueueMask;

            //---

            struct Entry
            {
                void* payload = nullptr;
                uint32_t queueMask = 0;
                Entry* next = nullptr;
                Entry* prev = nullptr;
            };

            struct EntryFreeList
            {
                Entry* alloc();
                void free(Entry* entry);

            private:
                Entry* list = nullptr;
                std::atomic<int> numFree = 0;
                std::atomic<int> numAllocated = 0;
            };

            struct QueueState
            {
                void push(Entry* entry);
                void unlink(Entry* entry);
                Entry* pop(uint32_t mask);
                void inspect(const TQueueInspectorFunc& inspector) const;

            private:
                Entry* head = nullptr;
                Entry* tail = nullptr;
                std::atomic<int> numQueued = 0;
            };

            struct QueueCounters
            {
                QueueCounters(uint32_t numQueues);
                ~QueueCounters() {};

                void add(uint32_t mask);
                void sub(uint32_t mask);
                uint32_t nonZeroMask();

            private:
                std::atomic<uint32_t> m_counters[8];
                std::atomic<uint32_t> m_nonZeroMask;
                uint32_t m_numQueues;
            };

            EntryFreeList m_entryFreeList;
            QueueState m_queueList;
            QueueCounters m_queueCounters;

            pthread_mutex_t listMutex;
            pthread_cond_t listCond;

            volatile bool m_requestExit;

            //----
        };

    } // prv
} // base