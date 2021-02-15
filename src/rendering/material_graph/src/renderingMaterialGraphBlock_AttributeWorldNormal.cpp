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

    class MaterialGraphBlock_AttributeWorldNormal : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeWorldNormal, MaterialGraphBlock);

    public:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Normal"_id, MaterialOutputSocket().hideCaption());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            return compiler.vertexData(MaterialVertexDataType::WorldNormal);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeWorldNormal);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("WorldNormal").group("Attributes");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialAttribute");
        RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
    RTTI_END_TYPE();

    ///---

} // rendering
