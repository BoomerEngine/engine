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

class MaterialGraphBlock_VectorCross : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorCross, MaterialGraphBlock);

public:
    MaterialGraphBlock_VectorCross()
    {}

    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("A"_id, MaterialInputSocket().hideCaption());
        builder.socket("B"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "A"_id, base::Vector3(0, 0, 1)).conform(3);
        CodeChunk b = compiler.evalInput(this, "B"_id, base::Vector3(0, 0, 1)).conform(3);
        return CodeChunkOp::Cross(a, b);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorCross);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("cross(a,b)").group("Vector").name("Cross");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialVector");
    RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
