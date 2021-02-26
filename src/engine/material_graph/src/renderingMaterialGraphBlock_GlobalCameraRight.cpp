/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\globals #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE()

///---

class MaterialGraphBlock_GlobalCameraRight : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_GlobalCameraRight, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return CodeChunk(CodeChunkType::Numerical3, "CameraParams.CameraRight");
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_GlobalCameraRight);
    RTTI_METADATA(graph::BlockInfoMetadata).title("Camera Right").group("Globals");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialGlobal");
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
