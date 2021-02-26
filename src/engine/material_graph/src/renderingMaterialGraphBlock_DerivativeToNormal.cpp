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

class MaterialGraphBlock_DerivativeToNormal : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_DerivativeToNormal, MaterialGraphBlock);

public:
    MaterialGraphBlock_DerivativeToNormal()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Normal"_id, MaterialOutputSocket());
        builder.socket("DXDY"_id, MaterialInputSocket());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk dxdy = compiler.evalInput(this, "DXDY"_id, Vector2(0,0)).conform(2);
        CodeChunk invZSquared = (dxdy.x() * dxdy.x()) + (dxdy.y() * dxdy.y()) + 1.0f;
        CodeChunk z = 1.0f / invZSquared.sqrt();
        return Float3(dxdy.x() * z, dxdy.y() * z, z);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_DerivativeToNormal);
    RTTI_METADATA(graph::BlockInfoMetadata).title("dx/dy to normal").group("Functions");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGeneric");
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
