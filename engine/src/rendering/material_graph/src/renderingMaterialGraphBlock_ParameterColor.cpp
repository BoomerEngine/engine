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
#include "rendering/material/include/renderingMaterialRuntimeLayout.h"

namespace rendering
{
    ///---

    class MaterialGraphBlock_ParameterColor : public MaterialGraphBlockParameter
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ParameterColor, MaterialGraphBlockParameter);

    public:
        MaterialGraphBlock_ParameterColor()
            : m_value(255,255,255,255)
        {}

        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Color"_id, MaterialOutputSocket());
            builder.socket("Alpha"_id, MaterialOutputSocket().swizzle("w"_id));
        }

        virtual MaterialDataLayoutParameterType parameterType() const override
        {
            return MaterialDataLayoutParameterType::Color;
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            if (const auto* entry = compiler.findParamEntry(name()))
            {
                if (entry->type == MaterialDataLayoutParameterType::Color)
                {
                    return CodeChunk(CodeChunkType::Numerical4, base::TempString("{}.{}", compiler.dataLayout()->descriptorName(), entry->name), true);
                }
            }

            return CodeChunk(m_value.toVectorSRGB());
        }

        virtual base::Type dataType() const override
        {
            return base::reflection::GetTypeObject<base::Color>();
        }

        virtual const void* dataValue() const override
        {
            return &m_value;
        }

        virtual void onPropertyChanged(base::StringView path) override
        {
            TBaseClass::onPropertyChanged(path);
        }

    private:
        base::Color m_value;
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ParameterColor);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("Color").group("Parameters").name("Color");
        RTTI_PROPERTY(m_value).editable("Default color value");
        RTTI_METADATA(base::graph::BlockShapeMetadata).slantedWithTitle();
    RTTI_END_TYPE();

    ///---

} // rendering
