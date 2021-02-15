/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "object.h"
#include "objectGlobalRegistry.h"

#include "base/object/include/object.h"
#include "base/system/include/scopeLock.h"

namespace base
{
    ///--

    ObjectGlobalRegistry::ObjectGlobalRegistry()
        : m_objectEntryPool(65536 / sizeof(ObjectEntry))
    {
        memset(m_objectHashBuckets, 0, sizeof(m_objectHashBuckets));
    }

    void ObjectGlobalRegistry::deinit()
    {
        
    }

    ObjectPtr ObjectGlobalRegistry::findObject(uint32_t id)
    {
        auto lock = CreateLock(m_objectsLock);

        // locate the entry
        const auto bucketIndex = id % MAX_BUCKETS;
        auto* curEntry = m_objectHashBuckets[bucketIndex];
        while (curEntry)
        {
            if (curEntry->id == id)
                return curEntry->ptr.lock();
            curEntry = curEntry->nextBucket;
        }

        // object not found
        return nullptr;
    }

    bool ObjectGlobalRegistry::iterateAllObjects(const std::function<bool(IObject*)>& enumFunc)
    {
        base::Array<ObjectWeakPtr> allObjects;

        // extract objects
        {
            auto lock = CreateLock(m_objectsLock);
            allObjects.reserve(m_objectCount);

            auto cur = m_objectListTail;
            while (cur)
            {
                allObjects.pushBack(cur->ptr);
                cur = cur->prevAll;
            }
        }

        // run enumerator
        for (auto& objPtr : allObjects)
            if (auto obj = objPtr.lock())
                if (enumFunc(obj))
                    return true;

        return false;
    }

    bool ObjectGlobalRegistry::iterateObjectsOfClass(ClassType objectClass, const std::function<bool(IObject*)>& enumFunc)
    {
        auto lock = CreateLock(m_objectsLock);

        auto cur = m_objectListTail;
        while (cur)
        {
            if (auto obj = cur->ptr.lock())
                if (obj->is(objectClass))
                    if (enumFunc(obj))
                        return true;

            cur = cur->prevAll;
        }

        return false;
    }

    void ObjectGlobalRegistry::registerObject(uint32_t id, IObject* object)
    {
        auto lock = CreateLock(m_objectsLock);

        // allocate entry
        m_objectCount += 1;
        auto* entry = m_objectEntryPool.alloc();
        memset(entry, 0, sizeof(ObjectEntry));
        entry->id = id;
        entry->ptr = object;
        entry->nextAll = nullptr;
        entry->prevAll= nullptr;
        entry->nextBucket  = nullptr;

        // link in the global list
        if (m_objectListHead != nullptr)
        {
            m_objectListHead->prevAll = entry;
            entry->nextAll = m_objectListHead;
        }
        else
        {
            m_objectListTail = entry;
        }

        m_objectListHead = entry;

        // add entry to the hash buckets
        const auto bucketIndex = id % MAX_BUCKETS;
        entry->nextBucket = m_objectHashBuckets[bucketIndex];
        m_objectHashBuckets[bucketIndex] = entry;
    }

    void ObjectGlobalRegistry::unregisterObject(uint32_t id, IObject* object)
    {
        auto lock = CreateLock(m_objectsLock);

        // locate the entry
        const auto bucketIndex = id % MAX_BUCKETS;
        auto* prevLink = &m_objectHashBuckets[bucketIndex];
        auto* curEntry = m_objectHashBuckets[bucketIndex];
        while (curEntry)
        {
            if (curEntry->id == id)
                break;
            prevLink = &curEntry->nextBucket;
            curEntry = curEntry->nextBucket;
        }

        // object was not registered
        DEBUG_CHECK_EX(curEntry != nullptr, TempString("Object ID {} not found in global registry", id));
        if (curEntry == nullptr)
            return;

        // verify the object
        DEBUG_CHECK_EX(curEntry->ptr.expired() || curEntry->ptr == object, TempString("Object ID {} is different that previoulsy registered", id));

        // remove from hash bucket
        *prevLink = curEntry->nextBucket;

        // remove from object list
        if (curEntry->nextAll)
        {
            DEBUG_CHECK(curEntry != m_objectListTail);
            curEntry->nextAll->prevAll = curEntry->prevAll;
        }
        else
        {
            DEBUG_CHECK(curEntry == m_objectListTail);
            m_objectListTail = curEntry->prevAll;
        }

        if (curEntry->prevAll)
        {
            DEBUG_CHECK(curEntry != m_objectListHead);
            curEntry->prevAll->nextAll = curEntry->nextAll;
        }
        else
        {
            DEBUG_CHECK(curEntry == m_objectListHead);
            m_objectListHead = curEntry->nextAll;
        }

        // release entry to memory pool
        curEntry->ptr.reset();
        m_objectEntryPool.release(curEntry);
        m_objectCount -= 1;
    }

    //--

} // base

