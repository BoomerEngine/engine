/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\stream #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE(rendering)

///---

class MaterialGraphBlock_StreamVertexUV : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_StreamVertexUV, MaterialGraphBlock);

public:
    virtual void buildLayout(base::graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("UV"_id, MaterialOutputSocket().hideCaption());
    }

    virtual base::StringBuf chooseTitle() const override
    {
        if (m_coordinate != 0)
            return base::TempString("VertexUV [{}]", m_coordinate);
        return TBaseClass::chooseTitle();
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, base::StringID outputName) const override
    {
        if (m_coordinate == 0)
            return compiler.vertexData(MaterialVertexDataType::VertexUV0);
        else if (m_coordinate == 1)
            return compiler.vertexData(MaterialVertexDataType::VertexUV1);
        else if (m_coordinate == 2)
            return compiler.vertexData(MaterialVertexDataType::VertexUV2);
        else if (m_coordinate == 3)
            return compiler.vertexData(MaterialVertexDataType::VertexUV3);
            
        return CodeChunk(base::Vector2(0, 0));
    }

private:
    uint8_t m_coordinate;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_StreamVertexUV);
    RTTI_METADATA(base::graph::BlockInfoMetadata).title("VertexUV").group("Streams");
    RTTI_METADATA(base::graph::BlockStyleNameMetadata).style("MaterialStream");
    RTTI_PROPERTY(m_coordinate).editable("Texture coordinate stream to output");
    RTTI_METADATA(base::graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE(rendering)
