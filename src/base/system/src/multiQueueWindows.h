/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\winapi #]
* [#platform: windows #]
***/

#pragma once

#include "thread.h"
#include "spinLock.h"
#include "multiQueue.h"
#include "atomic.h"

BEGIN_BOOMER_NAMESPACE(base)

namespace prv
{

    /// WinAPI based queue
    class WinMultiQueue : public IMultiQueue
    {
    public:
        static WinMultiQueue* Create(uint32_t numQueues);

    private:
        WinMultiQueue(uint32_t numInternalQueues);
        virtual ~WinMultiQueue();

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
            void* m_payload;
            uint32_t m_queueMask;
            Entry* m_next;
            Entry* m_prev;

            INLINE Entry()
                : m_payload(nullptr)
                , m_queueMask(0)
                , m_next(nullptr)
                , m_prev(nullptr)
            {}
        };

        struct EntryFreeList
        {
            EntryFreeList();
            ~EntryFreeList() {};

            Entry* alloc();
            void free(Entry* entry);

        private:
            Entry* m_list;
            //SpinLock lock;
            std::atomic<int> m_numFree;
            std::atomic<int> m_numAllocated;
        };

        struct QueueState
        {
            QueueState();
            ~QueueState() {};

            void push(Entry* entry);
            void unlink(Entry* entry);
            Entry* pop(uint32_t mask);
            void inspect(const TQueueInspectorFunc& inspector) const;

        private:
            Entry* m_head;
            Entry* m_tail;
            std::atomic<int> m_numQueued;
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

        CRITICAL_SECTION m_listMutex;
        HANDLE* m_listCond;

        volatile bool m_requestExit;

        //----
    };

} // prv

END_BOOMER_NAMESPACE(base)