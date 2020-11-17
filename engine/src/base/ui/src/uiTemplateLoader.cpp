/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: misc #]
***/

#include "build.h"
#include "uiTemplateLoader.h"
#include "uiElement.h"
#include "base/xml/include/xmlDocument.h"

namespace ui
{
    //--

    class TemplateClassRegistry : public base::ISingleton
    {
        DECLARE_SINGLETON(TemplateClassRegistry);

    public:
        TemplateClassRegistry()
        {
            base::InplaceArray<base::SpecificClassType<IElement>, 100> elementClasses;
            RTTI::GetInstance().enumClasses(elementClasses);

            for (auto classPtr : elementClasses)
            {
                if (auto classNameMetaData = static_cast<const ElementClassNameMetadata*>(classPtr->MetadataContainer::metadata(ElementClassNameMetadata::GetStaticClass())))
                {
                    TRACE_INFO("Mapped UI element '{}' as class '{}'", classNameMetaData->name(), classPtr);
                    m_classMap[classNameMetaData->name()] = classPtr;
                }
            }
        }

        base::SpecificClassType<IElement> findClass(base::StringView className) const
        {
            base::SpecificClassType<IElement> ret = nullptr;
            m_classMap.find(base::StringBuf(className), ret);
            return ret;
        }

    private:
        base::HashMap<base::StringBuf, base::SpecificClassType<IElement>> m_classMap;

        virtual void deinit() override
        {
            m_classMap.clear();
        }
    };


    //--

    TemplateLoader::TemplateLoader(base::StringView contextName, const base::xml::IDocument& xml)
        : m_xml(xml)
        , m_contextName(contextName)
    {}

    TemplateLoader::~TemplateLoader()
    {}

    bool TemplateLoader::finalize()
    {
        bool valid = true;

        auto elems = std::move(m_createdElements);
        for (const auto& elem : elems)
            valid &= elem->handleTemplateFinalize();

        return valid;
    }

    bool TemplateLoader::createElement(const base::xml::NodeID id, ElementPtr& outElem, base::HashMap<base::StringView, IElement*>& outNamedElements)
    {
        // validate the class
        auto className = m_xml.nodeName(id);
        auto classPtr = TemplateClassRegistry::GetInstance().findClass(className);
        if (!classPtr)
            return reportError(id, base::TempString("Unrecognized ui class name '{}'", className));

        // create the element
        auto element = classPtr->create<IElement>();
        if (!element)
            return reportError(id, base::TempString("Unable to create UI element of class '{}' ({})", className, classPtr->name()));

        // initialize the element
        if (!loadElement(id, element, outNamedElements))
            return false;

        outElem = element;
        return true;
    }

    bool TemplateLoader::loadElement(const base::xml::NodeID id, IElement*  elem, base::HashMap<base::StringView, IElement*>& outNamedElements)
    {
        // validate the class
        auto className = m_xml.nodeName(id);
        auto classPtr = TemplateClassRegistry::GetInstance().findClass(className);
        if (!classPtr)
            return reportError(id, base::TempString("Unrecognized ui class name '{}'", className));

        // make sure the item class is valid
        if (!elem->cls()->is(classPtr))
            return reportError(id, base::TempString("Existing UI element of class '{}' is not compatible with declared class '{}'", elem->cls()->name(), className));

        // load all attributes
        if (!loadAttributes(id, elem))
            return false;

        // load child items
        base::HashMap<base::StringView, IElement*> childNamedElements;
        if (!loadChildren(id, elem, childNamedElements))
            return false;

        // make sure all children were bound
        if (!elem->bindNamedProperties(childNamedElements))
            return reportError(id, "Unable to bind all required named elements");

        // merge list into the parent list (allows local elements to have scope of named children)
        for (auto* elem : childNamedElements.values())
            outNamedElements[elem->name().view()] = elem;

        return true;
    }

    bool TemplateLoader::loadAttributes(const base::xml::NodeID id, IElement*  elem)
    {
        auto attrId = m_xml.nodeFirstAttribute(id);
        while (attrId != 0)
        {
            if (!loadAttribute(id, attrId, elem))
                return false;
            attrId = m_xml.nextAttribute(attrId);
        }
        return true;
    }

    bool TemplateLoader::loadChildren(const base::xml::NodeID id, IElement*  elem, base::HashMap<base::StringView, IElement*>& outNamedElements)
    {
        auto childId = m_xml.nodeFirstChild(id);
        while (childId != 0)
        {
            if (!loadChild(id, childId, elem, outNamedElements))
                return false;
            childId = m_xml.nodeSibling(childId);
        }

        return true;
    }

    bool TemplateLoader::loadChild(const base::xml::NodeID id, const base::xml::NodeID childId, IElement*  elem, base::HashMap<base::StringView, IElement*>& outNamedElements)
    {
        auto childName = m_xml.nodeName(childId);

        if (elem->handleTemplateChild(childName, m_xml, childId))
            return true;

        ElementPtr childElement;
        if (!createElement(childId, childElement, outNamedElements))
            return false;

        if (!childElement->name().empty())
            outNamedElements[childElement->name().view()] = childElement.get();

        if (childElement)
        {
            if (elem->handleTemplateNewChild(m_xml, id, childId, childElement))
                m_createdElements.pushBack(childElement);
        }

        return true;
    }


    bool TemplateLoader::loadAttribute(const base::xml::NodeID id, const base::xml::AttributeID attrId, IElement*  elem)
    {
        auto nodeName = m_xml.nodeName(id);
        auto attrName = m_xml.attributeName(attrId);
        auto attrValue = m_xml.attributeValue(attrId);

        if (elem->handleTemplateProperty(attrName, attrValue))
            return true;

        reportError(id, base::TempString("Unrecognized attribute '{}', value '{}'", attrName, attrValue));
        return true;
    }

    bool TemplateLoader::reportError(const base::xml::NodeID id, base::StringView f)
    {
        uint32_t line = 0, pos = 0;
        m_xml.nodeLocationInfo(id, line, pos);

        TRACE_ERROR("{}({}): error: {}", m_contextName, line, f);
        return false;
    }

    //--

    bool ApplyTemplate(ui::IElement* ptr, base::StringView contextName, const base::xml::IDocument& doc)
    {
        if (!ptr)
            return false;

        base::HashMap<base::StringView, IElement*> namedElements;

        TemplateLoader loader(contextName, doc);
        if (!loader.loadElement(doc.root(), ptr, namedElements))
            return false;

        return loader.finalize();
    }

    bool ApplyTemplate(ui::IElement* ptr, const base::storage::XMLDataPtr& data)
    {
        DEBUG_CHECK_EX(data, "Missing template");
        if (!data)
            return false;

        return ApplyTemplate(ptr, data->path(), data->document());
    }

    ElementPtr LoadTemplate(base::StringView contextName, const base::xml::IDocument& doc)
    {
        TemplateLoader loader(contextName, doc);

        base::HashMap<base::StringView, IElement*> namedElements;

        ElementPtr ret;
        if (!loader.createElement(doc.root(), ret, namedElements))
            return nullptr;

        loader.finalize();
        return ret;
    }

    ElementPtr LoadTemplate(const base::storage::XMLDataPtr& data)
    {
        if (!data)
            return nullptr;

        return LoadTemplate(data->path(), data->document());
    }

    //--

} // ui
