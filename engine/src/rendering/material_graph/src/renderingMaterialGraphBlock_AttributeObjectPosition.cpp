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

    class MaterialGraphBlock_AttributeObjectPosition : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeObjectPosition, MaterialGraphBlock);

    public:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Position"_id, MaterialOutputSocket().hideCaption());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            return base::Vector3(0, 0, 0);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeObjectPosition);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("ObjectPosition").group("Attributes");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialAttribute");
        RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
    RTTI_END_TYPE();

    ///---

} // rendering
