/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types\class #]
***/

#include "build.h"
#include "rttiType.h"
#include "rttiProperty.h"

namespace base
{
    namespace rtti
    {

        //--

        Property::Property(const IType* parent, const PropertySetup& setup)
            : m_type(setup.m_type)
            , m_name(setup.m_name)
            , m_category(setup.m_category)
            , m_offset(setup.m_offset)
            , m_flags(setup.m_flags)
            , m_parent(parent)
        {
            CRC64  crc;
            crc << parent->name(); // must be from string!
            crc << m_name; // must be from string!
            m_hash = crc.crc();
        }

        Property::~Property()
        {
        }

    } // rtti
} // base
