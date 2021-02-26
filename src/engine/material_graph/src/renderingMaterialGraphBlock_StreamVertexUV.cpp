/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\stream #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE()

///---

class MaterialGraphBlock_StreamVertexUV : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_StreamVertexUV, MaterialGraphBlock);

public:
    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("UV"_id, MaterialOutputSocket().hideCaption());
    }

    virtual StringBuf chooseTitle() const override
    {
        if (m_coordinate != 0)
            return TempString("VertexUV [{}]", m_coordinate);
        return TBaseClass::chooseTitle();
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        if (m_coordinate == 0)
            return compiler.vertexData(MaterialVertexDataType::VertexUV0);
        else if (m_coordinate == 1)
            return compiler.vertexData(MaterialVertexDataType::VertexUV1);
        else if (m_coordinate == 2)
            return compiler.vertexData(MaterialVertexDataType::VertexUV2);
        else if (m_coordinate == 3)
            return compiler.vertexData(MaterialVertexDataType::VertexUV3);
            
        return CodeChunk(Vector2(0, 0));
    }

private:
    uint8_t m_coordinate;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_StreamVertexUV);
    RTTI_METADATA(graph::BlockInfoMetadata).title("VertexUV").group("Streams");
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialStream");
    RTTI_PROPERTY(m_coordinate).editable("Texture coordinate stream to output");
    RTTI_METADATA(graph::BlockShapeMetadata).rectangle();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
