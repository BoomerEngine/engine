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

class MaterialGraphBlock_AttributeObjectPosition : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeObjectPosition, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Position"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return Vector3(0, 0, 0);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeObjectPosition);
    RTTI_METADATA(graph::BlockInfoMetadata).title("ObjectPosition").group("Attributes");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialAttribute");
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
