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
#include "rendering/texture/include/renderingStaticTexture.h"
#include "rendering/material/include/renderingMaterialRuntimeLayout.h"

namespace rendering
{
    ///---

    class MaterialGraphBlock_ParameterTexture : public MaterialGraphBlockParameter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ParameterTexture, MaterialGraphBlockParameter);

    public:
        MaterialGraphBlock_ParameterTexture()
        {}

        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Texture"_id, MaterialOutputSocket().tag("TEXTURE"_id).color(COLOR_SOCKET_TEXTURE).hideCaption());
        }

        virtual MaterialDataLayoutParameterType parameterType() const override
        {
            return MaterialDataLayoutParameterType::Texture2D;
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            if (const auto* entry = compiler.findParamEntry(name()))
            {
                if (entry->type == MaterialDataLayoutParameterType::Texture2D)
                {
                    return CodeChunk(CodeChunkType::TextureResource, base::TempString("{}.{}", compiler.dataLayout()->descriptorName(), entry->name), true);
                }
            }
            
            return CodeChunk();
        }

        virtual base::Type dataType() const override
        {
            return base::reflection::GetTypeObject<rendering::TextureRef>();
        }

        virtual const void* dataValue() const override
        {
            return &m_value;
        }

        virtual void onPropertyChanged(base::StringView<char> path) override
        {
            TBaseClass::onPropertyChanged(path);
        }

    private:
        rendering::TextureRef m_value;
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ParameterTexture);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("Texture").group("Parameters").name("Texture");
        RTTI_PROPERTY(m_value).editable("Default texture");
        RTTI_METADATA(base::graph::BlockShapeMetadata).slantedWithTitle();
    RTTI_END_TYPE();

    ///---

} // rendering
