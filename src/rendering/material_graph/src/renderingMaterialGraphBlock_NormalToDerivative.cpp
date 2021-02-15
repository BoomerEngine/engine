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
    using namespace CodeChunkOp;

    ///---

    class MaterialGraphBlock_NormalToDerivative : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_NormalToDerivative, MaterialGraphBlock);

    public:
        MaterialGraphBlock_NormalToDerivative()
        {}

        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("DXDY"_id, MaterialOutputSocket());
            builder.socket("Normal"_id, MaterialInputSocket());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            CodeChunk normal = compiler.evalInput(this, "Normal"_id, base::Vector3(0,0,1)).conform(3);
            return Float2(normal.x() / normal.z(), normal.y() / normal.z());
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_NormalToDerivative);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("Normal to dx/dy").group("Functions");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialGeneric");
    RTTI_END_TYPE();

    ///---

} // rendering
