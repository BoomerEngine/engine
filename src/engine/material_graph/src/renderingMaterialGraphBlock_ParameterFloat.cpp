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
#include "renderingMaterialCode.h"
#include "engine/material/include/renderingMaterialRuntimeLayout.h"

BEGIN_BOOMER_NAMESPACE()

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

END_BOOMER_NAMESPACE()