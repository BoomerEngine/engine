/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\constants #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE()

///---

class MaterialGraphBlock_ConstFloat : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_ConstFloat, MaterialGraphBlock);

public:
    MaterialGraphBlock_ConstFloat()
        : m_value(0.0f)
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Value"_id, MaterialOutputSocket().hideCaption());
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        return CodeChunk(m_value);
    }

private:
    float m_value;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_ConstFloat);
    RTTI_METADATA(graph::BlockInfoMetadata).title("Scalar").group("Constants").name("Constant Scalar");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialConst");
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();
    RTTI_PROPERTY(m_value).editable("Constant value");
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
