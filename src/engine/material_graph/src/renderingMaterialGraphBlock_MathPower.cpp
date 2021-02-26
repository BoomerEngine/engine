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

class MaterialGraphBlock_MathPower : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathPower, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket());
        builder.socket("E"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk x = compiler.evalInput(this, "X"_id, 0.0f);
        CodeChunk e = compiler.evalInput(this, "E"_id, 1.0f);
        e = e.conform(x.components());
        return x.pow(e);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathPower);
    RTTI_METADATA(graph::BlockInfoMetadata).title("pow(x,e)").group("Math");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
    RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
