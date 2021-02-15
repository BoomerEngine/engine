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

    class MaterialGraphBlock_VectorNormalize : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_VectorNormalize, MaterialGraphBlock);

    public:
        MaterialGraphBlock_VectorNormalize()
        {}

        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Out"_id, MaterialOutputSocket().hideCaption());
            builder.socket("X"_id, MaterialInputSocket().hideCaption());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            CodeChunk a = compiler.evalInput(this, "In"_id, base::Vector3(0, 0, 1));
            if (a.components() == 2 || a.components() == 3 || a.components() == 4)
                return CodeChunkOp::Normalize(a);
            else
                return 1.0f;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_VectorNormalize);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("normalize(x)").group("Vector").name("Normalize");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialVector");
        RTTI_METADATA(base::graph::BlockShapeMetadata).slanted();
    RTTI_END_TYPE();

    ///---

} // rendering
