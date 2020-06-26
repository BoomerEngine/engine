/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: reflection #]
***/

#include "build.h"
#include "reflectionPropertyBuilder.h"
#include "propertyDecorators.h"

#include "base/object/include/rttiMetadata.h"
#include "base/object/include/rttiProperty.h"

namespace base
{
    namespace reflection
    {

        PropertyBuilder::PropertyBuilder(Type propType, const char* name, const char* category, uint32_t offset)
            : m_type(propType)
            , m_name(name)
            , m_category(category)
            , m_offset(offset)
        {
        }

        PropertyBuilder::~PropertyBuilder()
        {
        }

        void PropertyBuilder::submit(rtti::IClassType* targetClass)
        {
            uint32_t flags = 0;

            rtti::PropertySetup setup;
            setup.m_name = m_name;
            setup.m_category = m_category;
            setup.m_offset = m_offset;
            setup.m_type = m_type;

            if (m_editable)
                setup.m_flags |= rtti::PropertyFlagBit::Editable;
            if (m_readonly)
                setup.m_flags |= rtti::PropertyFlagBit::ReadOnly;
            if (m_inlined)
                setup.m_flags |= rtti::PropertyFlagBit::Inlined;
            if (m_scriptHidden)
                setup.m_flags |= rtti::PropertyFlagBit::ScriptHidden;
            if (m_scriptReadOnly)
                setup.m_flags |= rtti::PropertyFlagBit::ScriptReadOnly;
            if (m_transient)
                setup.m_flags |= rtti::PropertyFlagBit::Transient;
            

            auto prop  = MemNew(rtti::Property, targetClass, setup);

            for (auto metaData  : m_metadata)
                prop->attachMetadata(metaData);

            if (m_editable && m_comment)
                static_cast<PropertyCommentMetadata&>(prop->addMetadata(PropertyCommentMetadata::GetStaticClass())).text = m_comment;

            targetClass->addProperty(prop);
        }

    } // reflection
} // base