/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\objects #]
***/

#pragma once

#include "rendering/driver/include/renderingObject.h"

namespace rendering
{
    namespace gl4
    {
        class Driver;
        class ObjectPool;
        
        // gl4 object type (mini-RTTI)
        enum class ObjectType : uint8_t
        {
            Invalid,

            Buffer,
            Image,
            Parameters,
            Sampler,
            ShaderLibraryAdapter,
            Output,
        };

        // wrapper for a gl4 objects
        TYPE_ALIGN(4, class) Object : public base::NoCopy
        {
        public:
            Object(Driver* drv, ObjectType type);
            virtual ~Object(); // called on rendering thread once all frames this object was used are done rendering

            // get simple value representing object type (RTTI), needed for lame casts
            INLINE ObjectType objectType() const { return m_type; }

            // get the original driver used to create this object
            INLINE Driver* driver() const { return m_driver; }

            // get the handle to this object
            INLINE ObjectID handle() const { return ObjectID(ObjectID::STATIC, (void*)this); }

            // mark this object for deletion
            bool markForDeletion();

        private:
            Driver* m_driver;
            ObjectType m_type;
            std::atomic<uint32_t> m_markedForDeletion;
        };

        // resolve object ID to high-level static object, fatal asserts if the type is not valid
        // NOTE: this ONLY works for memory manged objects but it also does not require a context
        template< typename T >
        static INLINE T* ResolveStaticObject(const ObjectID& id)
        {
            // empty or transient objects cannot be resolved
            if (!id.isStatic())
                return nullptr;

            // get the object wrapper
            auto object  = static_cast<Object*>(id.internalPointer());
            ASSERT_EX(T::CheckClassType(object->objectType()), "Unexpected object type");
            return static_cast<T*>(object);
        }

    } // gl4
} // driver