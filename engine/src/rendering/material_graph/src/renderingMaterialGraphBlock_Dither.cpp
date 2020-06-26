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
    using namespace CodeChunkOp;

    ///---

    class MaterialGraphBlock_Dither : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_Dither, MaterialGraphBlock);

    public:
        MaterialGraphBlock_Dither()
        {}

        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Out"_id, MaterialOutputSocket());
            builder.socket("In"_id, MaterialInputSocket());
            builder.socket("Amount"_id, MaterialInputSocket());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            CodeChunk color = compiler.evalInput(this, "In"_id, base::Vector3(0,0,0)).conform(3);
            // TODO
            return color;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_Dither);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("Dither").group("Functions");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialGeneric");
    RTTI_END_TYPE();

    ///---

} // rendering
