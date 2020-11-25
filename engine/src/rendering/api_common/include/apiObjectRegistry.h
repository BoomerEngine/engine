/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

namespace rendering
{
    namespace api
    {
        //--

        class ObjectRegistryProxy;

        /// internal object registry
        class RENDERING_API_COMMON_API ObjectRegistry : public IDeviceObjectHandler // external client proxies have have weak referencs to this
        {
            RTTI_DECLARE_POOL(POOL_API_OBJECTS)

        public:
            ObjectRegistry(IBaseThread* owner);
            virtual ~ObjectRegistry();

            //--

			// owner, the device thread
			INLINE IBaseThread* owner() const { return m_owner; }

            //--

            // allocate object ID and bind object
            ObjectID registerObject(IBaseObject* ptr);

            // unregister object after it was deleted
            void unregisterObject(ObjectID id, IBaseObject* ptr);

            //--

            // request object to be deleted when this frame ends
            void requestObjectDeletion(ObjectID id);

            //--

            // resolve static object
			IBaseObject* resolveStatic(ObjectID id, ObjectType expectedType = ObjectType::Unknown) const;

            // resolve static object
            template< typename T >
            INLINE T* resolveStatic(ObjectID id) const
            {
                return static_cast<T*>(resolveStatic(id, T::STATIC_TYPE));
            }

        private:
            static const uint32_t MAX_OBJECTS = 1U << 16;

            base::Mutex m_lock; // may be called from threads

            uint32_t m_numAllocatedObjects = 0;

            struct Entry
            {
                IBaseObject* ptr = nullptr;
                bool markedForDeletion = false;
            };

            uint32_t m_numObjects = 0;
            Entry* m_objects = nullptr;

            base::Array<uint32_t> m_freeEntries;
            uint32_t m_generationCounter = 1;

            IBaseThread* m_owner = nullptr;

			//--

			virtual void releaseToDevice(ObjectID id) override final;
			virtual api::IBaseObject* resolveInternalObjectPtrRaw(ObjectID id, uint8_t objectType) override final;
        };

        //--

    } // gl4
} // rendering