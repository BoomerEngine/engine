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

class MaterialGraphBlock_AttributeObjectColor : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeObjectColor, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("RGB"_id, MaterialOutputSocket().swizzle("xyz"_id));
        builder.socket("R"_id, MaterialOutputSocket().swizzle("x"_id).hiddenByDefault());
        builder.socket("G"_id, MaterialOutputSocket().swizzle("y"_id).hiddenByDefault());
        builder.socket("B"_id, MaterialOutputSocket().swizzle("z"_id).hiddenByDefault());
        builder.socket("A"_id, MaterialOutputSocket().swizzle("w"_id));
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return Vector4(1,0,1,0);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeObjectColor);
    RTTI_METADATA(graph::BlockInfoMetadata).title("ObjectColor").group("Attributes");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialAttribute");
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()