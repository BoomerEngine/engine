/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\globals #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

class MaterialGraphBlock_GlobalEngineTime : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_GlobalEngineTime, MaterialGraphBlock);

public:
    MaterialGraphBlock_GlobalEngineTime()
    {}

    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        return CodeChunk(CodeChunkType::Numerical1, "FrameParams.EngineTime");
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_GlobalEngineTime);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("Engine time").group("Globals");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialGlobal");
    RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
