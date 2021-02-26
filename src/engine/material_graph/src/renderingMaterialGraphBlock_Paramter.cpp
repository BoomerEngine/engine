/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks #]
***/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphBlock_Parameter.h"

BEGIN_BOOMER_NAMESPACE()

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialGraphBlockParameter);
    RTTI_CATEGORY("Parameter");
    RTTI_PROPERTY(m_name).editable("Name of the parameter");
    RTTI_PROPERTY(m_category).editable("Display category in the material");
        RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialParameter");
RTTI_END_TYPE();

MaterialGraphBlockParameter::MaterialGraphBlockParameter()
    : m_category("Parameters"_id)
{}

MaterialGraphBlockParameter::MaterialGraphBlockParameter(StringID name)
    : m_name(name)
    , m_category("Parameters"_id)
{}

bool MaterialGraphBlockParameter::resetValue()
{
    const auto* defaultData = ((const MaterialGraphBlockParameter*)cls()->defaultObject())->dataValue();
    return writeValue(defaultData, dataType());
}

bool MaterialGraphBlockParameter::writeValue(const void* data, Type type)
{
    if (rtti::ConvertData(data, type, (void*)dataValue(), dataType()))
    {
        postEvent(EVENT_OBJECT_PROPERTY_CHANGED, StringBuf("value"));
        return true;
    }

    return false;
}

bool MaterialGraphBlockParameter::readValue(void* data, Type type, bool defaultValueOnly /*= false*/) const
{
    if (defaultValueOnly)
    {
        const auto* defaultData = ((const MaterialGraphBlockParameter*)cls()->defaultObject())->dataValue();
        return rtti::ConvertData(defaultData, dataType(), data, type);
    }
    else
    {
        return rtti::ConvertData(dataValue(), dataType(), data, type);
    }
}

///---

StringBuf MaterialGraphBlockParameter::chooseTitle() const
{
    if (m_name)
        return TempString("{} [{}]", TBaseClass::chooseTitle(), m_name);
    else
        return TBaseClass::chooseTitle();
}

void MaterialGraphBlockParameter::onPropertyChanged(StringView path)
{
    if (path == "name" || path == "category")
    {
        invalidateStyle();

        if (auto graph = findParent<MaterialGraphContainer>())
            graph->refreshParameterList();
    }
    else if (path == "value" && !name().empty())
    {
        if (auto graph = findParent<MaterialGraph>())
            graph->onPropertyChanged(name().view());
    }

    return TBaseClass::onPropertyChanged(path);
}

///---

END_BOOMER_NAMESPACE()
