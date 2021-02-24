/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\math #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

class MaterialGraphBlock_MathFloor : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathFloor, MaterialGraphBlock);

public:
    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return CodeChunkOp::Floor(a);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathFloor);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("floor(x)").group("Math");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialMath");
    RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
