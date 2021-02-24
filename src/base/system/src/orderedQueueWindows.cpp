/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\threading\winapi #]
* [#platform: winapi #]
***/

#include "build.h"
#include "orderedQueueWindows.h"

#include <Windows.h>

BEGIN_BOOMER_NAMESPACE(base)

namespace prv
{

    //--

    WinOrderedQueue* WinOrderedQueue::Create()
    {
        return new WinOrderedQueue();
    }

    //--

    WinOrderedQueue::WinOrderedQueue()
        : m_requestExit(false)
        , m_entryFreeList(4096)
        , m_orderBucketFreeList(1024)
    {
        // initialize synchronization stuff
        InitializeCriticalSection(&m_listMutex);

        // create queue semaphores
        m_listSemaphore = CreateSemaphore(NULL, 0, INT_MAX, NULL);
    }

    WinOrderedQueue::~WinOrderedQueue()
    {
        DeleteCriticalSection(&m_listMutex);
        CloseHandle(m_listSemaphore);
    }

    void WinOrderedQueue::close()
    {
        m_requestExit = true;
        ReleaseSemaphore(m_listSemaphore, 256, NULL);
    }

    void WinOrderedQueue::push(void* jobData, uint64_t order)
    {
        EnterCriticalSection(&m_listMutex);

        auto* entry = m_entryFreeList.alloc();
        entry->payload = jobData;
        entry->next = nullptr;
        entry->prev = nullptr;

        auto* bucket = getBucket(order);
        if (bucket->tail)
        {
            entry->prev = bucket->tail;
            bucket->tail->next = entry;
            bucket->tail = entry;
        }
        else
        {
            bucket->tail = entry;
            bucket->head = entry;
        }

        LeaveCriticalSection(&m_listMutex);

        ReleaseSemaphore(m_listSemaphore, 1, NULL);
    }

    void* WinOrderedQueue::pop()
    {
        // wait for some work to be there
        auto ret = WaitForSingleObject(m_listSemaphore, INFINITE);

        // we are kindly requested to exit
        if (m_requestExit || ret != WAIT_OBJECT_0)
                return nullptr;

        EnterCriticalSection(&m_listMutex);

        // look for the entry in the ordered buckets
        for (auto* bucket = m_orderList.head; bucket; bucket = bucket->next)
        {
            if (auto* entry = bucket->head)
            {
                bucket->head = entry->next;
                if (bucket->head)
                    bucket->head->prev = nullptr;
                else
                    bucket->tail = nullptr;

                auto* ret = entry->payload;
                m_entryFreeList.release(entry);

                if (bucket->head == nullptr && !bucket->hot)
                {
                    unlinkBucket(bucket);
                    m_orderBucketFreeList.release(bucket);
                }

                LeaveCriticalSection(&m_listMutex);
                return ret;
            }
        }

        LeaveCriticalSection(&m_listMutex);
        return nullptr;
    }

    void WinOrderedQueue::inspect(const TQueueInspectorFunc& inspectorFunc)
    {

    }

    //--

    void WinOrderedQueue::unlinkBucket(OrderBucket* bucket)
    {
        if (bucket->next)
        {
            ASSERT(m_orderList.tail != bucket);
            bucket->next->prev = bucket->prev;
        }
        else
        {
            ASSERT(m_orderList.tail == bucket);
            m_orderList.tail = bucket->prev;
        }

        if (bucket->prev)
        {
            ASSERT(m_orderList.head != bucket);
            bucket->prev->next = bucket->next;
        }
        else
        {
            ASSERT(m_orderList.head == bucket);
            m_orderList.head = bucket->next;
        }

        uint64_t prevOrder = 0;
        for (auto* cur = m_orderList.head; cur; cur = cur->next)
        {
            DEBUG_CHECK(prevOrder < cur->order);
            DEBUG_CHECK(cur->next || cur == m_orderList.tail);
            DEBUG_CHECK(cur->prev || cur == m_orderList.head);
            prevOrder = cur->order;
        }
    }

    void WinOrderedQueue::linkBucket(OrderBucket* bucket)
    {
        DEBUG_CHECK(bucket->next == nullptr);
        DEBUG_CHECK(bucket->prev == nullptr);
        DEBUG_CHECK(bucket->head == nullptr);
        DEBUG_CHECK(bucket->tail == nullptr);

        if (m_orderList.head == nullptr)
        {
            m_orderList.head = bucket;
            m_orderList.tail = bucket;
            return;
        }

        auto* addBefore = m_orderList.head;
        while (addBefore && bucket->order > addBefore->order)
            addBefore = addBefore->next;

        if (addBefore)
        {
            if (auto* addAfter = addBefore->prev)
            {
                bucket->prev = addAfter;
                bucket->next = addBefore;
                addAfter->next = bucket;
                addBefore->prev = bucket;
            }
            else
            {
                if (m_orderList.head)
                    m_orderList.head->prev = bucket;
                bucket->next = m_orderList.head;
                m_orderList.head = bucket;
            }
        }
        else
        {
            if (m_orderList.tail) 
                m_orderList.tail->next = bucket;
            bucket->prev = m_orderList.tail;
            m_orderList.tail = bucket;
        }

        uint64_t prevOrder = 0;
        for (auto* cur = m_orderList.head; cur; cur = cur->next)
        {
            DEBUG_CHECK(prevOrder < cur->order);
            DEBUG_CHECK(cur->next || cur == m_orderList.tail);
            DEBUG_CHECK(cur->prev || cur == m_orderList.head);
            prevOrder = cur->order;
        }
    }

    WinOrderedQueue::OrderBucket* WinOrderedQueue::getBucket(uint64_t orderList)
    {
        // check in the hot buckets
        uint32_t minBucketIndex = 0;
        uint64_t minBucketValue = ~0ULL;
        for (uint32_t i = 0; i < OrderList::MAX_HOT_BUCKETS; ++i)
        {
            const auto& hot = m_orderList.hotBuckets[i];
            if (hot.order == orderList && hot.bucket)
                return hot.bucket;

            if (hot.order < minBucketValue)
            {
                minBucketValue = hot.order;
                minBucketIndex = i;
            }
        }

        // if we are evicting bucket and it has no jobs we can remove it
        auto& evictedHot = m_orderList.hotBuckets[minBucketIndex];
        if (auto* bucketToEvict = evictedHot.bucket)
        {
            DEBUG_CHECK(bucketToEvict->hot);
            bucketToEvict->hot = false;

            evictedHot.bucket = nullptr;

            if (!bucketToEvict->head)
            {
                unlinkBucket(bucketToEvict);
                m_orderBucketFreeList.release(bucketToEvict);
            }
        }

        // do a linear search to find existing bucket
        auto* bucket = m_orderList.head;
        while (nullptr != bucket)
        {
            if (bucket->order == orderList)
                break;
            bucket = bucket->next;
        }

        // no matching bucket found, create a new one
        if (!bucket)
        {
            bucket = m_orderBucketFreeList.alloc();
            bucket->order = orderList;
            bucket->next = nullptr;
            bucket->prev = nullptr;
            bucket->head = nullptr;
            bucket->tail = nullptr;
            bucket->hot = false;
            linkBucket(bucket);
        }

        // make hot
        DEBUG_CHECK(!bucket->hot);
        bucket->hot = true;

        // replace in hot list
        evictedHot.bucket = bucket;
        evictedHot.order = orderList;
        return bucket;
    }

    //--

} // prv

END_BOOMER_NAMESPACE(base)