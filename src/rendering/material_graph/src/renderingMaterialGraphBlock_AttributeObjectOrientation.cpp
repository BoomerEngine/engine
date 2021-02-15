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

    class MaterialGraphBlock_AttributeObjectOrientation : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeObjectOrientation, MaterialGraphBlock);

    public:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Forward"_id, MaterialOutputSocket());
            builder.socket("Up"_id, MaterialOutputSocket().hiddenByDefault());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            return base::Vector3(0, 0, 0);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeObjectOrientation);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("ObjectOrientation").group("Attributes");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialAttribute");
        RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
    RTTI_END_TYPE();

    ///---

} // rendering
