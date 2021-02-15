/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: xml #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/xml/include/xmlDocument.h"

namespace base
{
    namespace storage
    {

        /// a resource-type wrapper for external XML files
        /// NOTE: in cooked version the data is saved as compressed binary XML to save space
        class BASE_STORAGE_API XMLData : public res::IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(XMLData, res::IResource)

        public:
            XMLData();
            XMLData(const xml::DocumentPtr& document);

            /// get the XML document interface
            /// NOTE: do not cache!
            INLINE const xml::IDocument& document() { return *m_document; }

            //--

        private:
            xml::DocumentPtr m_document;
            Buffer m_binaryData;
        };

    } // storage
} // base

