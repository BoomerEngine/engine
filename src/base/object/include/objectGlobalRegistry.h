/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

#include "object.h"
#include "rttiDataHolder.h"

#include "base/system/include/spinLock.h"
#include "base/system/include/mutex.h"
#include "base/system/include/atomic.h"
#include "base/containers/include/hashMap.h"
#include "base/system/include/simpleStructurePool.h"

BEGIN_BOOMER_NAMESPACE(base)

/// Global list of all objects that derive from IObject
class BASE_OBJECT_API ObjectGlobalRegistry : public ISingleton
{
    DECLARE_SINGLETON(ObjectGlobalRegistry);

public:
    ObjectGlobalRegistry();

    ///---

    // get total object count
    INLINE uint32_t totalObjectCount() const { return m_objectCount.load(); }

    ///---

    /// get object by ID
    ObjectPtr findObject(uint32_t id);

    ///---

    /// visit all objects with a function on EACH OBJECT, do I have to say it will be slow ? :) 
    /// NOTE: registry is locked during this update 
    bool iterateAllObjects(const std::function<bool(IObject*)>& enumFunc);

    /// visit all objects of specific class with a function, still slow
    /// NOTE: registry is locked during this update 
    bool iterateObjectsOfClass(ClassType objectClass, const std::function<bool(IObject*)>& enumFunc);

    /// visit all objects of specific class with a function, still slow
    /// NOTE: registry is locked during this update 
    template< typename T >
    INLINE bool iterateObjectsOfClass(const std::function<bool(T*)>& enumFunc)
    {
        return iterateObjectsOfClass(T::GetStaticClass(), *(const std::function<bool(IObject*)>*) & enumFunc);
    }

    ///---

protected:
    // register object in the registry
    void registerObject(uint32_t id, IObject* object);

    // unregister object from the registry
    void unregisterObject(uint32_t id, IObject* object);

    //--

    static const uint32_t MAX_BUCKETS = 16 * 1024;

    struct ObjectEntry
    {
        uint32_t id = 0;
        ObjectWeakPtr ptr;
        ObjectEntry* nextBucket = nullptr;
        ObjectEntry* nextAll = nullptr;
        ObjectEntry* prevAll = nullptr;
    };

    SpinLock m_objectsLock;
    ObjectEntry* m_objectListHead = nullptr;
    ObjectEntry* m_objectListTail = nullptr;

    SimpleStructurePool<ObjectEntry> m_objectEntryPool;
    ObjectEntry* m_objectHashBuckets[MAX_BUCKETS];

    std::atomic<uint32_t> m_objectCount = 0;


    //--

    virtual void deinit() override;

    //-

    friend class IObject;
};

END_BOOMER_NAMESPACE(base)

