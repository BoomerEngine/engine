/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\attribute #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

namespace rendering
{
    ///---

    class MaterialGraphBlock_AttributePixelPosition : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributePixelPosition, MaterialGraphBlock);

    public:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Position"_id, MaterialOutputSocket().hideCaption());
            builder.socket("X"_id, MaterialOutputSocket().swizzle("x"_id).hiddenByDefault());
            builder.socket("Y"_id, MaterialOutputSocket().swizzle("y"_id).hiddenByDefault());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            if (compiler.stage() == ShaderType::Pixel)
                return CodeChunk(CodeChunkType::Numerical2, "(gl_FragCoord.xy)");
            else
                return base::Vector2(0.0f, 0.0f);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributePixelPosition);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("PixelPosition").group("Attributes");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialAttribute");
        RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
    RTTI_END_TYPE();

    ///---

} // rendering
