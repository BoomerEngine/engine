/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\math #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE()

///---

class MaterialGraphBlock_MathMin : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathMin, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, 0.0f);
        CodeChunk b = compiler.evalInput(this, "B"_id, 0.0f);

        const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
        a = a.conform(maxComponents);
        b = b.conform(maxComponents);

        return CodeChunkOp::Min(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathMin);
    RTTI_METADATA(graph::BlockInfoMetadata).title("min(a,b)").group("Math");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
    RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
