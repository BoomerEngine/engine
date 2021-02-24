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

BEGIN_BOOMER_NAMESPACE(rendering)

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(MaterialGraphBlockParameter);
    RTTI_CATEGORY("Parameter");
    RTTI_PROPERTY(m_name).editable("Name of the parameter");
    RTTI_PROPERTY(m_category).editable("Display category in the material");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialParameter");
RTTI_END_TYPE();

MaterialGraphBlockParameter::MaterialGraphBlockParameter()
    : m_category("Parameters"_id)
{}

MaterialGraphBlockParameter::MaterialGraphBlockParameter(base::StringID name)
    : m_name(name)
    , m_category("Parameters"_id)
{}

bool MaterialGraphBlockParameter::resetValue()
{
    const auto* defaultData = ((const MaterialGraphBlockParameter*)cls()->defaultObject())->dataValue();
    return writeValue(defaultData, dataType());
}

bool MaterialGraphBlockParameter::writeValue(const void* data, base::Type type)
{
    if (base::rtti::ConvertData(data, type, (void*)dataValue(), dataType()))
    {
        postEvent(base::EVENT_OBJECT_PROPERTY_CHANGED, base::StringBuf("value"));
        return true;
    }

    return false;
}

bool MaterialGraphBlockParameter::readValue(void* data, base::Type type, bool defaultValueOnly /*= false*/) const
{
    if (defaultValueOnly)
    {
        const auto* defaultData = ((const MaterialGraphBlockParameter*)cls()->defaultObject())->dataValue();
        return base::rtti::ConvertData(defaultData, dataType(), data, type);
    }
    else
    {
        return base::rtti::ConvertData(dataValue(), dataType(), data, type);
    }
}

///---

base::StringBuf MaterialGraphBlockParameter::chooseTitle() const
{
    if (m_name)
        return base::TempString("{} [{}]", TBaseClass::chooseTitle(), m_name);
    else
        return TBaseClass::chooseTitle();
}

void MaterialGraphBlockParameter::onPropertyChanged(base::StringView path)
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

END_BOOMER_NAMESPACE(rendering)
