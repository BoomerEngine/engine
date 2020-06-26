/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#pragma once

#include "uiElement.h"

namespace ui
{

    /// load UI template from XML file
    class BASE_UI_API TemplateLoader : public base::NoCopy
    {
    public:
        TemplateLoader(base::StringView<char> contextName, const base::xml::IDocument& xml);
        ~TemplateLoader();

        bool finalize();

        bool createElement(const base::xml::NodeID id, ElementPtr& outElem, base::HashMap<base::StringView<char>, IElement*>& outNamedElements);

        bool loadElement(const base::xml::NodeID id, IElement* elem, base::HashMap<base::StringView<char>, IElement*>& outNamedElements);

        bool loadAttributes(const base::xml::NodeID id, IElement* elem);
        bool loadAttribute(const base::xml::NodeID id, const base::xml::AttributeID attrId, IElement* elem);

        bool loadChildren(const base::xml::NodeID id, IElement* elem, base::HashMap<base::StringView<char>, IElement*>& outNamedElements);
        bool loadChild(const base::xml::NodeID id, const base::xml::NodeID childId, IElement* elem, base::HashMap<base::StringView<char>, IElement*>& outNamedElements);

    private:
        const base::xml::IDocument& m_xml;
        base::StringView<char> m_contextName;

        base::Array<ElementPtr> m_createdElements;

        bool reportError(const base::xml::NodeID id, base::StringView<char> f);
    };

} // ui