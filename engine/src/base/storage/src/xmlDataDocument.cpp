/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: xml #]
***/

#include "build.h"
#include "xmlDataDocument.h"

namespace base
{
    namespace storage
    {
        ///--

        mem::PoolID POOL_XML("Engine.XML");

        ///--

        RTTI_BEGIN_TYPE_CLASS(XMLData);
            RTTI_METADATA(base::res::ResourceExtensionMetadata).extension("v4xml");
            RTTI_METADATA(base::res::ResourceDescriptionMetadata).description("Binary XML");
            RTTI_PROPERTY(m_binaryData);
        RTTI_END_TYPE();

        XMLData::XMLData()
        {}

        XMLData::XMLData(const xml::DocumentPtr& document)
            : m_document(document)
        {
            StringBuilder txt;
            m_document->saveAsText(txt, m_document->root());
            m_binaryData = Buffer::Create(POOL_XML, txt.length(), 1, txt.c_str());
        }

        ///---

    } // storage
} // base