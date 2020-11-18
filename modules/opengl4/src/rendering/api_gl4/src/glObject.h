/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/device/include/renderingObject.h"

namespace rendering
{
    namespace gl4
    {
        //--

        // wrapper for a gl4 objects
        class Object : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_OBJECTS)

        public:
            Object(Device* drv, ObjectType type);
            virtual ~Object(); // called on rendering thread once all frames this object was used are done rendering

            INLINE ObjectType objectType() const { return m_type; }
            INLINE Device* device() const { return m_device; }
            INLINE ObjectID handle() const { return m_handle; }

        private:
            Device* m_device;
            ObjectType m_type;
            ObjectID m_handle;
        };

        //--

    } // gl4
} // rendering