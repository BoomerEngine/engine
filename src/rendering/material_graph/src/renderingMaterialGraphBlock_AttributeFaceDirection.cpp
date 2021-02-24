/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\attribute #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

class MaterialGraphBlock_AttributeFaceDirection : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_AttributeFaceDirection, MaterialGraphBlock);

public:
    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Side"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        if (compiler.stage() == ShaderStage::Pixel)
            return CodeChunk(CodeChunkType::Numerical1, base::TempString("(gl_FrontFacing ? {} : {})", m_frontValue, m_backValue));
        else
            return m_frontValue;
    }

    float m_frontValue = 1.0f;
    float m_backValue = 0.0f;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_AttributeFaceDirection);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("FaceDirection").group("Attributes");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialAttribute");
    RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
    RTTI_PROPERTY(m_frontValue).editable("Value to output when front facing");
    RTTI_PROPERTY(m_backValue).editable("Value to output when back facing (only when two sided rendering is enabled)");
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
