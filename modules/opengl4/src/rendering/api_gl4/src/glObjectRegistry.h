/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

namespace rendering
{
    namespace gl4
    {
        //--

        class ObjectRegistryProxy;

        /// internal object registry
        class ObjectRegistry : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_OBJECTS)

        public:
            ObjectRegistry(Device* drv, DeviceThread* drvThread);
            ~ObjectRegistry();

            //--

            // handler interface
            INLINE ObjectRegistryProxy* proxy() const { return m_proxy; }

            //--

            // allocate object ID and bind object
            ObjectID registerObject(Object* ptr);

            // unregister object after it was deleted
            void unregisterObject(ObjectID id, Object* ptr);

            //--

            // request object to be deleted when this frame ends
            void requestObjectDeletion(ObjectID id);

            // resolve object run function with it, makes sure object is NOT deleted by anything
            bool runWithObject(ObjectID id, const std::function<void(Object*)>& func);

            //--

            // resolve static object
            Object* resolveStatic(ObjectID id, ObjectType expectedType) const;

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
                Object* ptr = nullptr;
                bool markedForDeletion = false;
            };

            uint32_t m_numObjects = 0;
            Entry* m_objects = nullptr;

            base::Array<uint32_t> m_freeEntries;
            uint32_t m_generationCounter = 1;

            Device* m_device = nullptr;
            DeviceThread* m_thread = nullptr;

            base::RefPtr<ObjectRegistryProxy> m_proxy;
        };

        //--

        class ObjectRegistryProxy : public rendering::IDeviceObjectHandler
        {
        public:
            ObjectRegistryProxy(ObjectRegistry* target);

            void disconnect();

            virtual void releaseToDevice(ObjectID id) override;

			//--

			Object* resolveStatic(ObjectID id, ObjectType type) const;

			template< typename T >
			INLINE T* resolveStatic(ObjectID id) const
			{
				return static_cast<T*>(resolveStatic(id, T::STATIC_TYPE));
			}

			//--

			bool runWithObject(ObjectID id, const std::function<void(Object*)>& func) const;

            template< typename T >
            bool run(ObjectID id, const std::function<void(T*)>& func) const
            {
                return runWithObject(id, [&func](Object* obj) { func(static_cast<T*>(obj)); });
            }

			//--

        private:
            base::Mutex m_lock;
            ObjectRegistry* m_registry = nullptr;
        };

        //--

    } // gl4
} // rendering