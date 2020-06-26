/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\objects #]
***/

#include "build.h"
#include "glObject.h"

namespace rendering
{
    namespace gl4
    {
        //--

        Object::Object(Driver* drv, ObjectType type)
            : m_driver(drv)
            , m_type(type)
        {}

        Object::~Object()
        {}

        bool Object::markForDeletion()
        {
            return 0 == m_markedForDeletion.exchange(1);
        }

        //--

    } // gl4
} // driver
