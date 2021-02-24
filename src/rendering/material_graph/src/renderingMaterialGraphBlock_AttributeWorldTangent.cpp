/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\attribute #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

class MaterialGraphBlock_AttributeWorldTangent : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeWorldTangent, MaterialGraphBlock);

public:
    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Tangent"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        return compiler.vertexData(MaterialVertexDataType::WorldTangent);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeWorldTangent);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("WorldTangent").group("Attributes");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialAttribute");
    RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
