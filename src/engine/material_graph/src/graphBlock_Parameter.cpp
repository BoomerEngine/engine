/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\parameters #]
***/

#include "build.h"
#include "graph.h"
#include "graphBlock_Parameter.h"
#include "engine/material/include/runtimeLayout.h"
#include "engine/texture/include/texture.h"

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

class MaterialGraphBlock_ParameterColor : public MaterialGraphBlockParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ParameterColor, MaterialGraphBlockParameter);

public:
    MaterialGraphBlock_ParameterColor()
        : m_value(255,255,255,255)
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Color"_id, MaterialOutputSocket());
        builder.socket("R"_id, MaterialOutputSocket().swizzle("x"_id).hiddenByDefault());
        builder.socket("G"_id, MaterialOutputSocket().swizzle("y"_id).hiddenByDefault());
        builder.socket("B"_id, MaterialOutputSocket().swizzle("z"_id).hiddenByDefault());
        builder.socket("Alpha"_id, MaterialOutputSocket().swizzle("w"_id));
    }

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Color;
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (const auto* entry = compiler.findParamEntry(name()))
        {
            if (entry->type == MaterialDataLayoutParameterType::Color)
            {
				if (compiler.context().bindlessTextures)
				{

				}
				else
				{
					const auto& layout = compiler.dataLayout()->discreteDataLayout();
					return CodeChunk(CodeChunkType::Numerical4, TempString("{}.{}", layout.descriptorName, entry->name), true);
				}
            }
        }

        return CodeChunk(m_value.toVectorSRGB());
    }

    virtual Type dataType() const override
    {
        return reflection::GetTypeObject<Color>();
    }

    virtual const void* dataValue() const override
    {
        return &m_value;
    }

    virtual void onPropertyChanged(StringView path) override
    {
        TBaseClass::onPropertyChanged(path);
    }

private:
    Color m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ParameterColor);
    RTTI_METADATA(graph::BlockInfoMetadata).title("Color").group("Parameters").name("Color");
    RTTI_PROPERTY(m_value).editable("Default color value");
    RTTI_METADATA(graph::BlockShapeMetadata).slantedWithTitle();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_ParameterFloat : public MaterialGraphBlockParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ParameterFloat, MaterialGraphBlockParameter);

public:
    MaterialGraphBlock_ParameterFloat()
            : m_value(0.0f)
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Float;
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (const auto* entry = compiler.findParamEntry(name()))
        {
            if (entry->type == MaterialDataLayoutParameterType::Float)
            {
                if (compiler.context().bindlessTextures)
                {

                }
                else
                {
                    const auto& layout = compiler.dataLayout()->discreteDataLayout();
                    return CodeChunk(CodeChunkType::Numerical1, TempString("{}.{}", layout.descriptorName, entry->name), true);
                }
            }
        }

        // default to a constant value
        return CodeChunk(m_value);
    }

    virtual Type dataType() const override
    {
        return reflection::GetTypeObject<float>();
    }

    virtual const void* dataValue() const override
    {
        return &m_value;
    }

    virtual void onPropertyChanged(StringView path) override
    {
        TBaseClass::onPropertyChanged(path);
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ParameterFloat);
RTTI_METADATA(graph::BlockInfoMetadata).title("Scalar").group("Parameters").name("Scalar");
RTTI_PROPERTY(m_value).editable("Default value");
RTTI_METADATA(graph::BlockShapeMetadata).slantedWithTitle();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_ParameterTexture : public MaterialGraphBlockParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ParameterTexture, MaterialGraphBlockParameter);

public:
    MaterialGraphBlock_ParameterTexture()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Texture"_id, MaterialOutputSocket().tag("TEXTURE"_id).color(COLOR_SOCKET_TEXTURE).hideCaption());
    }

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Texture2D;
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (const auto* entry = compiler.findParamEntry(name()))
        {
            if (entry->type == MaterialDataLayoutParameterType::Texture2D)
            {
                if (compiler.context().bindlessTextures)
                {

                }
                else
                {
                    const auto& layout = compiler.dataLayout()->discreteDataLayout();
                    return CodeChunk(CodeChunkType::TextureResource, TempString("{}.{}", layout.descriptorName, entry->name), true);
                }
            }
        }

        return CodeChunk();
    }

    virtual Type dataType() const override
    {
        return reflection::GetTypeObject<TextureRef>();
    }

    virtual const void* dataValue() const override
    {
        return &m_value;
    }

    virtual void onPropertyChanged(StringView path) override
    {
        TBaseClass::onPropertyChanged(path);
    }

private:
    TextureRef m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ParameterTexture);
RTTI_METADATA(graph::BlockInfoMetadata).title("Texture").group("Parameters").name("Texture");
RTTI_PROPERTY(m_value).editable("Default texture");
RTTI_METADATA(graph::BlockShapeMetadata).slantedWithTitle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
