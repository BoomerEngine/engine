/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\globals #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

namespace rendering
{
    ///---

    class MaterialGraphBlock_GlobalCameraDirection : public MaterialGraphBlock
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_GlobalCameraDirection, MaterialGraphBlock);

    public:
        virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
        {
            builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
        }

        virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
        {
            return CodeChunk(CodeChunkType::Numerical3, "CameraParams.CameraForward");
        }

    private:
        float m_value;
    };

    RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_GlobalCameraDirection);
        RTTI_METADATA(base::graph::BlockInfoMetadata).title("Camera Direction").group("Globals");
        RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialGlobal");
        RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
    RTTI_END_TYPE();

    ///---

} // rendering
