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

class MaterialGraphBlock_AttributePixelPosition : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributePixelPosition, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Position"_id, MaterialOutputSocket().hideCaption());
        builder.socket("X"_id, MaterialOutputSocket().swizzle("x"_id).hiddenByDefault());
        builder.socket("Y"_id, MaterialOutputSocket().swizzle("y"_id).hiddenByDefault());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (compiler.stage() == gpu::ShaderStage::Pixel)
            return CodeChunk(CodeChunkType::Numerical2, "(gl_FragCoord.xy)");
        else
            return Vector2(0.0f, 0.0f);
    }
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributePixelPosition);
    RTTI_METADATA(graph::BlockInfoMetadata).title("PixelPosition").group("Attributes");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialAttribute");
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
