/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\generic #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

namespace rendering
{
    ///---

    class MaterialGraphBlock_SmoothStep : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_SmoothStep, MaterialGraphBlock);

    public:
        MaterialGraphBlock_SmoothStep()
        {}

        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Out"_id, MaterialOutputSocket());
            builder.socket("X"_id, MaterialInputSocket());
            builder.socket("Start"_id, MaterialInputSocket());
            builder.socket("End"_id, MaterialInputSocket());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            CodeChunk a = compiler.evalInput(this, "X"_id, 0.5f);
            CodeChunk start = compiler.evalInput(this, "Start"_id, 0.0f);
            CodeChunk end = compiler.evalInput(this, "End"_id, 1.0f);

            const auto comps = a.components();
            start = start.conform(comps);
            end = end.conform(comps);

            return CodeChunkOp::SmoothStep(start, end, a);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_SmoothStep);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("SmoothStep").group("Functions");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialGeneric");
    RTTI_END_TYPE();

    ///---

} // rendering
