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
#include "orderedQueue.h"
#include "atomic.h"
#include "simpleStructurePool.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{
    /// WinAPI based queue
    class WinOrderedQueue : public IOrderedQueue
    {
    public:
        static WinOrderedQueue* Create();

    private:
        WinOrderedQueue();
        virtual ~WinOrderedQueue();

        virtual void close() override final;
        virtual void push(void* jobData, uint64_t order) override final;
        virtual void* pop() override final;
        virtual void inspect(const TQueueInspectorFunc& inspectorFunc) override final;

        //---

        struct Entry
        {
            void* payload = nullptr;
            Entry* next = nullptr;
            Entry* prev = nullptr;
        };

        struct OrderBucket
        {
            uint64_t order = 0;
            bool hot = false;
            OrderBucket* next = nullptr;
            OrderBucket* prev = nullptr;

            Entry* head = nullptr;
            Entry* tail = nullptr;

            void push(Entry* entry);
            void unlink(Entry* entry);
            Entry* pop(uint32_t mask);
            void inspect(const TQueueInspectorFunc& inspector) const;
        };

        //--

        struct HotOrderListEntry
        {
            uint64_t order = 0;
            OrderBucket* bucket = nullptr;
        };

        struct OrderList
        {
            OrderBucket* head = nullptr;
            OrderBucket* tail = nullptr;

            static const auto MAX_HOT_BUCKETS = 8;
            HotOrderListEntry hotBuckets[MAX_HOT_BUCKETS];
        };

        void unlinkBucket(OrderBucket* bucket);
        void linkBucket(OrderBucket* bucket);
        OrderBucket* getBucket(uint64_t orderList);

        //--

        SimpleStructurePool<Entry> m_entryFreeList;
        SimpleStructurePool<OrderBucket> m_orderBucketFreeList;
        OrderList m_orderList;

        CRITICAL_SECTION m_listMutex;
        HANDLE m_listSemaphore;

        volatile bool m_requestExit;

        //----
    };

} // prv

END_BOOMER_NAMESPACE()
