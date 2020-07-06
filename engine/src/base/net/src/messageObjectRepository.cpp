/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#include "build.h"
#include "messageObjectRepository.h"

namespace base
{
    namespace net
    {
        //--

        MessageObjectRepository::MessageObjectRepository()
        {
            m_objectMap.reserve(1024);
            m_objectReverseMap.reserve(1024);
            m_objectMap[0] = nullptr;
            m_objectReverseMap[nullptr] = 0; // just for consistency
            m_freeObjectIds.reserve(256);
        }

        MessageObjectRepository::~MessageObjectRepository()
        {}

        replication::DataMappedID MessageObjectRepository::allocateObjectId()
        {
            auto lock = CreateLock(m_lock);
            return allocateObjectId_NoLock();
        }

        replication::DataMappedID MessageObjectRepository::allocateObjectId_NoLock()
        {
            replication::DataMappedID id = 0;
            if (m_freeObjectIds.empty())
            {
                id = m_nextObjectId++;
            }
            else
            {
                id = m_freeObjectIds.back();
                m_freeObjectIds.popBack();
            }

            m_allocatedIds.insert(id);
            return (replication::DataMappedID)id;
        }

        replication::DataMappedID MessageObjectRepository::attachNewObject(const ObjectPtr& object)
        {
            if (!object)
                return 0;

            auto lock = CreateLock(m_lock);
            auto id = allocateObjectId_NoLock();
            if (!id)
                return 0;

            m_objectMap[id] = object;
            m_objectReverseMap[object.get()] = id;
            return id;
        }


        void MessageObjectRepository::attachObject(const replication::DataMappedID id, const ObjectPtr& object)
        {
            if (!id || !object)
                return;

            auto lock = CreateLock(m_lock);

            m_objectMap[id] = object;
            m_objectReverseMap[object.get()] = id;
        }

        void MessageObjectRepository::detachObject(const replication::DataMappedID id, bool freeId/* = true*/)
        {
            if (!id)
                return;

            auto lock = CreateLock(m_lock);

            ObjectWeakPtr ptr;
            if (m_objectMap.find(id, ptr))
                m_objectReverseMap.remove(ptr.unsafe());
            m_objectMap.remove(id);

            if (freeId)
            {
                if (m_allocatedIds.remove(id))
                    m_freeObjectIds.pushBack(id);
            }
        }

        ObjectPtr MessageObjectRepository::resolveObject(const replication::DataMappedID id) const
        {
            if (!id)
                return nullptr;

            auto lock = CreateLock(m_lock);

            ObjectWeakPtr ptr;
            if (m_objectMap.find(id, ptr))
                return ptr.lock();

            return nullptr;
        }

        IObject* MessageObjectRepository::resolveObjectPtr(const replication::DataMappedID id) const
        {
            if (!id)
                return nullptr;

            auto lock = CreateLock(m_lock);

            ObjectWeakPtr ptr;
            if (m_objectMap.find(id, ptr))
                return ptr.unsafe();

            return nullptr;
        }

        replication::DataMappedID MessageObjectRepository::findObjectId(const IObject* obj) const
        {
            if (!obj)
                return 0;

            replication::DataMappedID id = 0;
            if (m_objectReverseMap.find((void*)obj, id))
            {
                // TODO: remove this shit
                ObjectWeakPtr ptr;
                if (m_objectMap.find(id, ptr))
                {
                    if (ptr.unsafe() == obj)
                        return id;
                }
            }

            return 0;
        }

        //--

    } // msg
} // base

