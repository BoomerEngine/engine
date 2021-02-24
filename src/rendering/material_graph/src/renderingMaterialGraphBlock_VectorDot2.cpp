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

class MaterialGraphBlock_VectorDot2 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorDot2, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorDot2()
    {}

    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, base::Vector2(1, 1)).conform(2);
        CodeChunk b = compiler.evalInput(this, "B"_id, base::Vector2(1, 1)).conform(2);
        return CodeChunkOp::Dot2(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorDot2);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("dot2(a,b)").group("Vector").name("Dot2");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialVector");
    RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();

RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
