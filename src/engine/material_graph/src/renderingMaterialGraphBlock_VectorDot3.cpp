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

class MaterialGraphBlock_VectorDot3 : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorDot3, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorDot3()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, Vector3(1, 1, 1)).conform(3);
        CodeChunk b = compiler.evalInput(this, "B"_id, Vector3(1, 1, 1)).conform(3);
        return CodeChunkOp::Dot3(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorDot3);
    RTTI_METADATA(graph::BlockInfoMetadata).title("dot3(a,b)").group("Vector").name("Dot3");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialVector");
    RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
