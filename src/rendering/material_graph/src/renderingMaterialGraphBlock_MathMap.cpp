/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\math #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

namespace rendering
{
    ///---

    class MaterialGraphBlock_MathMap : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathMap, MaterialGraphBlock);

    public:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
            builder.socket("In"_id, MaterialInputSocket());
            builder.socket("Min"_id, MaterialInputSocket().hiddenByDefault());
            builder.socket("Max"_id, MaterialInputSocket().hiddenByDefault());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            CodeChunk a = compiler.evalInput(this, "In"_id, 0.0f);
            CodeChunk minV = compiler.evalInput(this, "Min"_id, m_defaultMin);
            CodeChunk maxV = compiler.evalInput(this, "Max"_id, m_defaultMax);

            minV = minV.conform(a.components());
            maxV = maxV.conform(a.components());

            return ((a - minV) / (maxV - minV)).saturate();
        }

        float m_defaultMin = 0.0f;
        float m_defaultMax = 1.0f;
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathMap);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("map(x)").group("Math");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialMath");
        RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
        RTTI_PROPERTY(m_defaultMin).editable("Minimum range value (in case custom value is not provided)");
        RTTI_PROPERTY(m_defaultMax).editable("Maximum range value (in case custom value is not provided)");
    RTTI_END_TYPE();

    ///---

} // rendering
