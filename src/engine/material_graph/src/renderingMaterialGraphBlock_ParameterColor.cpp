/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\parameters #]
***/

#include "build.h"
#include "renderingMaterialGraph.h"
#include "renderingMaterialGraphBlock_Parameter.h"
#include "engine/material/include/renderingMaterialRuntimeLayout.h"

BEGIN_BOOMER_NAMESPACE()

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

END_BOOMER_NAMESPACE()
