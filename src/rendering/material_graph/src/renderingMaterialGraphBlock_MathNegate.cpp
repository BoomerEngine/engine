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

class MaterialGraphBlock_MathNegate : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathNegate, MaterialGraphBlock);

public:
    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialInputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        CodeChunk a = compiler.evalInput(this, "X"_id, 0.0f);
        return -a;
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathNegate);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("-x").group("Math").name("Negate");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialMath");
    RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
