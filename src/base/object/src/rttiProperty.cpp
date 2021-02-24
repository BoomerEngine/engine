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

BEGIN_BOOMER_NAMESPACE(base::rtti)

//--

PropertyEditorData::PropertyEditorData()
{}

bool PropertyEditorData::rangeEnabled() const
{
    if (m_rangeMax < DBL_MAX)
        return true;
    if (m_rangeMin > -DBL_MAX)
        return true;
    return false;
}

//--

Property::Property(const IType* parent, const PropertySetup& setup, const PropertyEditorData& editorData)
    : m_type(setup.m_type)
    , m_name(setup.m_name)
    , m_category(setup.m_category)
    , m_offset(setup.m_offset)
    , m_flags(setup.m_flags)
    , m_parent(parent)
    , m_editorData(editorData)
{
    CRC64  crc;
    crc << parent->name(); // must be from string!
    crc << m_name; // must be from string!
    m_hash = crc.crc();
}

Property::~Property()
{
}

END_BOOMER_NAMESPACE(base::rtti)