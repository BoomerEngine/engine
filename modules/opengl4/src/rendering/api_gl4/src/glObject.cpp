/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"

#include "glDevice.h"
#include "glObject.h"
#include "glObjectRegistry.h"

namespace rendering
{
    namespace gl4
    {
        //--

        Object::Object(Device* drv, ObjectType type)
            : m_device(drv)
            , m_type(type)
        {
            m_handle = drv->objectRegistry().registerObject(this);
            DEBUG_CHECK(!m_handle.empty());
        }

        Object::~Object()
        {
            m_device->objectRegistry().unregisterObject(m_handle, this);
        }

        //--

    } // gl4
} // rendering
