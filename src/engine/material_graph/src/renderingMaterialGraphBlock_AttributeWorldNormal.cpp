/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\attribute #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE()

///---

class MaterialGraphBlock_AttributeWorldNormal : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeWorldNormal, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Normal"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return compiler.vertexData(MaterialVertexDataType::WorldNormal);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeWorldNormal);
    RTTI_METADATA(graph::BlockInfoMetadata).title("WorldNormal").group("Attributes");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialAttribute");
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
