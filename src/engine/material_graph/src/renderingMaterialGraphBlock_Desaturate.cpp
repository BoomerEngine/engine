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

using namespace CodeChunkOp;

///---

class MaterialGraphBlock_Desaturate : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_Desaturate, MaterialGraphBlock);

public:
    MaterialGraphBlock_Desaturate()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Out"_id, MaterialOutputSocket());
        builder.socket("In"_id, MaterialInputSocket());
        builder.socket("Amount"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk color = compiler.evalInput(this, "In"_id, Vector3(0,0,0)).conform(3);
        CodeChunk lum = Dot3(color, Vector3(0.299f, 0.587f, 0.114f)); // color perceptual luminance weights

        if (hasConnectionOnSocket("Amount"_id))
        {
            CodeChunk amount = compiler.evalInput(this, "Amount"_id, 0.5f);
            return Lerp(color, lum.xxx(), amount);
        }
        else
        {
            return lum.xxx();
        }            
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_Desaturate);
    RTTI_METADATA(graph::BlockInfoMetadata).title("Desaturate").group("Functions");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGeneric");
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
