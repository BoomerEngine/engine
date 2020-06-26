/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\math #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

namespace rendering
{
    ///---

    class MaterialGraphBlock_MathArcTan2 : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathArcTan2, MaterialGraphBlock);

    public:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
            builder.socket("X"_id, MaterialInputSocket());
            builder.socket("Y"_id, MaterialInputSocket());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            CodeChunk x = compiler.evalInput(this, "X"_id, 0.0f).x();
            CodeChunk y = compiler.evalInput(this, "Y"_id, 0.0f).x();
            return CodeChunkOp::ArcTan2(y,x);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathArcTan2);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("atan2(y,x)").group("Math");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialMath");
        RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
    RTTI_END_TYPE();

    ///---

} // rendering
