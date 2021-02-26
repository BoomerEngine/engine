/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: object #]
***/

#include "build.h"
#include "objectIndirectTemplate.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_STRUCT(ObjectIndirectTemplateProperty);
    RTTI_PROPERTY(name);
    RTTI_PROPERTY(type);
    RTTI_PROPERTY(data);
RTTI_END_TYPE();

ObjectIndirectTemplateProperty::ObjectIndirectTemplateProperty()
{}

//---

RTTI_BEGIN_TYPE_CLASS(ObjectIndirectTemplate);
    RTTI_PROPERTY(m_enabled);
    RTTI_PROPERTY(m_placement);
    RTTI_PROPERTY(m_templateClass);
    RTTI_PROPERTY(m_properties);
RTTI_END_TYPE();

ObjectIndirectTemplate::ObjectIndirectTemplate()
{}

ObjectIndirectTemplate::~ObjectIndirectTemplate()
{}

void ObjectIndirectTemplate::enable(bool flag, bool callEvent /*= true*/)
{
    if (m_enabled != flag)
    {
        m_enabled = flag;
        if (callEvent)
            onPropertyChanged("enabled");
    }
}

void ObjectIndirectTemplate::placement(const EulerTransform& placement, bool callEvent/*= true*/)
{
    if (m_placement != placement)
    {
        m_placement = placement;
        if (callEvent)
            onPropertyChanged("placement");
    }
}

void ObjectIndirectTemplate::templateClass(ClassType cls, bool callEvent)
{
    if (m_templateClass != cls)
    {
        m_templateClass = cls;
        if (callEvent)
        {
            onPropertyChanged("templateClass");
            postEvent(EVENT_OBJECT_STRUCTURE_CHANGED);
        }
    }
}

const ObjectIndirectTemplateProperty* ObjectIndirectTemplate::findProperty(StringID name) const
{
    for (const auto& prop : m_properties)
        if (prop.name == name)
            return &prop;

    return nullptr;
}

bool ObjectIndirectTemplate::removeProperty(StringID name)
{
    for (auto index : m_properties.indexRange())
    {
        if (m_properties[index].name == name)
        {
            m_properties.erase(index);
            onPropertyChanged(name.view());
            return true;
        }
    }

    return nullptr;
}

bool ObjectIndirectTemplate::writeProperty(StringID name, const void* data, const Type type)
{
    if (m_templateClass)
    {
        const rtti::TemplateProperty* templateProp = nullptr;
        for (const auto& prop : m_templateClass->allTemplateProperties())
        {
            if (prop.name == name)
            {
                templateProp = &prop;
                break;
            }
        }

        if (templateProp)
        {
            ObjectIndirectTemplateProperty* localProp = nullptr;

            for (auto& info : m_properties)
            {
                if (info.name == name)
                {
                    localProp = &info;
                    break;
                }
            }

            if (!localProp)
            {
                auto& entry = m_properties.emplaceBack();
                entry.name = templateProp->name;
                entry.type = templateProp->type;
                entry.data.init(templateProp->type, templateProp->defaultValue);
                localProp = &entry;
            }

            if (!rtti::ConvertData(data, type, localProp->data.data(), localProp->data.type()))
                return false;

            onPropertyChanged(name.view());
            return true;
        }
        else
        {
            TRACE_WARNING("Property '{}' not found in template class '{}'", name, m_templateClass);
        }
    }
    else
    {
        TRACE_WARNING("No template class, unable to set property '{}'", name);
    }

    return false;
}

bool ObjectIndirectTemplate::addProperty(const ObjectIndirectTemplateProperty& prop)
{
    DEBUG_CHECK_RETURN_EX_V(prop.name, "Invalid property name", false);
    DEBUG_CHECK_RETURN_EX_V(prop.data, "Invalid property data", false);
    DEBUG_CHECK_RETURN_EX_V(prop.type, "Invalid property type", false);

    removeProperty(prop.name);
    m_properties.pushBack(prop);
        
    markModified();
    return true;
}

bool ObjectIndirectTemplate::createProperty(const rtti::TemplateProperty* source)
{
    DEBUG_CHECK_RETURN_EX_V(source, "Invalid source property", nullptr);

    removeProperty(source->name);

    auto& entry = m_properties.emplaceBack();
    entry.name = source->name;
    entry.type = source->type;
    entry.data.init(source->type, source->defaultValue);

    markModified();

    return true;
}
    
//---

END_BOOMER_NAMESPACE()


