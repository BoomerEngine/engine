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

    class MaterialGraphBlock_MathSubtract : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_MathSubtract, MaterialGraphBlock);

    public:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
            builder.socket("A"_id, MaterialInputSocket());
            builder.socket("B"_id, MaterialInputSocket());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            CodeChunk a = compiler.evalInput(this, "A"_id, 0.0f);
            CodeChunk b = compiler.evalInput(this, "B"_id, 0.0f);

            const auto maxComponents = std::max<uint8_t>(a.components(), b.components());
            a = a.conform(maxComponents);
            b = b.conform(maxComponents);

            return a - b;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_MathSubtract);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("a - b").group("Math").name("Subtract");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialMath");
        RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
    RTTI_END_TYPE();

    ///---

} // rendering
