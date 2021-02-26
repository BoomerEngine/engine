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

class MaterialGraphBlock_MathDerivative : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathDerivative, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("dF/dx"_id, MaterialOutputSocket());
        builder.socket("dF/dy"_id, MaterialOutputSocket());
        builder.socket("F"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk f = compiler.evalInput(this, "F"_id, 0.0f);
        if (outputName == "dF/dx")
            return f.ddx();
        else if (outputName == "dF/dy")
            return f.ddy();
        else
            return 0.0f;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathDerivative);
    RTTI_METADATA(graph::BlockInfoMetadata).title("dF/d").group("Math");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialMath");
    RTTI_METADATA(graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
