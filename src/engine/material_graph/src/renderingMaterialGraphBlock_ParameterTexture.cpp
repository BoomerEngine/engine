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
#include "engine/texture/include/renderingStaticTexture.h"
#include "engine/material/include/renderingMaterialRuntimeLayout.h"

BEGIN_BOOMER_NAMESPACE()

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
