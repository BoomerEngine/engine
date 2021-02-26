/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\stream #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE()

///---

class MaterialGraphBlock_StreamVertexPosition : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_StreamVertexPosition, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Position"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return compiler.vertexData(MaterialVertexDataType::VertexPosition);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_StreamVertexPosition);
    RTTI_METADATA(graph::BlockInfoMetadata).title("VertexPosition").group("Streams");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialStream");
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
