/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#pragma once

#include "base/object/include/object.h"
#include "base/system/include/thread.h"
#include "base/system/include/spinLock.h"
#include "base/containers/include/bitPool.h"
#include "base/containers/include/hashMap.h"
#include "base/containers/include/hashSet.h"

namespace base
{
    namespace net
    {

        //--

        /// ID <-> object mapping, allows to translate pointers when sent in messages
        /// Thread safe
        class BASE_NET_API MessageObjectRepository : public IReferencable
        {
        public:
            MessageObjectRepository();
            ~MessageObjectRepository();

            //--

            // attach local object as target for messages with given ID, also can be used to resolve pointers to objects
            // NOTE: id 0 is reserved for the original "owner" of the peer connection
            // NOTE: peer has no authority to allocate those IDs, only server can
            void attachObject(const replication::DataMappedID id, const ObjectPtr& object);

            // remove previously registered object to handle messages
            // NOTE: this will auto matically  free the object ID back to pool if it was allocate from it
            void detachObject(const replication::DataMappedID id, bool freeId = true);

            //--

            // allocate unused ID for mapping object
            // NOTE: id 0 is always NULL, id 1 is kind of reserverd for the "host" object so we will start at 2 :)
            replication::DataMappedID allocateObjectId();

            // allocate object ID and attach the object, basically attachObject(allocateObjectId(), object) but done under one lock, not two
            replication::DataMappedID attachNewObject(const ObjectPtr& object);

            //--

            // resolve ID to an object
            ObjectPtr resolveObject(const replication::DataMappedID id) const;

            // resolve ID to an object, raw pointer version
            IObject* resolveObjectPtr(const replication::DataMappedID id) const;

            // get ID for object
            replication::DataMappedID findObjectId(const IObject* obj) const;

            //--

        private:
            SpinLock m_lock;

            HashMap<replication::DataMappedID, ObjectWeakPtr> m_objectMap;
            HashMap<void*, replication::DataMappedID> m_objectReverseMap;

            HashSet<replication::DataMappedID> m_allocatedIds;

            BitPool<> m_idAllocator;

            //--

            replication::DataMappedID allocateObjectId_NoLock();
        };

        //--

    } // msg
} // base