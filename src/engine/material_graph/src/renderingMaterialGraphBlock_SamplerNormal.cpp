/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\samplers #]
***/

#include "build.h"
#include "renderingMaterialGraphBlock.h"

BEGIN_BOOMER_NAMESPACE()

using namespace CodeChunkOp;

///---

class MaterialGraphBlock_SamplerNormal : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_SamplerNormal, MaterialGraphBlock);

public:
    MaterialGraphBlock_SamplerNormal()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Normal"_id, MaterialOutputSocket());

        builder.socket("UV"_id, MaterialInputSocket());
        builder.socket("Texture"_id, MaterialInputSocket().tag("TEXTURE"_id).color(Color::FIREBRICK));
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk sampler = compiler.evalInput(this, "Texture"_id, CodeChunk());
        if (sampler.empty())
            return CodeChunk(Vector4::ONE());

        CodeChunk uv;
        if (hasConnectionOnSocket("UV"_id))
            uv = compiler.evalInput(this, "UV"_id, CodeChunk(Vector2(0, 0))).conform(2);
        else
            uv = compiler.vertexData(MaterialVertexDataType::VertexUV0);


        auto textureXY = CodeChunk(CodeChunkType::Numerical2, TempString("texture({}, {}).xy", sampler, uv));
        auto normalXY = compiler.var(2.0f * textureXY - 1.0f);
        auto z = (1.0f - normalXY.x() * normalXY.x() - normalXY.y() * normalXY.y()).sqrt();

        if (m_normalScale != 1.0f)
            z = z * m_normalScale;

        auto normal = compiler.var(Float3(normalXY.x(), normalXY.y(), z));

        if (!m_sampleInTangentSpace)
        {
            auto baseT = compiler.vertexData(MaterialVertexDataType::WorldTangent);
            auto baseB = compiler.vertexData(MaterialVertexDataType::WorldBitangent);
            auto baseN = compiler.vertexData(MaterialVertexDataType::WorldNormal);
            normal = compiler.var(Normalize(baseT * normal.x() + baseB * normal.y() + baseN * normal.z()));
        }

        return normal;
    }

private:
    bool m_sampleInTangentSpace = false;
    float m_normalScale = 1.0f;
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_SamplerNormal);
    RTTI_METADATA(graph::BlockInfoMetadata).title("SamplerNormal").group("Samplers");
    RTTI_METADATA(graph::BlockTitleColorMetadata).color(COLOR_SOCKET_TEXTURE);
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialSampler");
    RTTI_PROPERTY(m_sampleInTangentSpace).editable("Output normal in tangent space rather than in world space");
    RTTI_PROPERTY(m_normalScale).editable("Scale the output normal");
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
