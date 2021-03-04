/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: blocks\samplers #]
***/

#include "build.h"
#include "graphBlock_Sampler.h"

BEGIN_BOOMER_NAMESPACE()

using namespace CodeChunkOp;

///---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IMaterialGraphBlockSamplerParameter);
    RTTI_PROPERTY(m_texCoordIndex).editable("UV set to use as a base, usually 0");
RTTI_END_TYPE();

IMaterialGraphBlockSamplerParameter::IMaterialGraphBlockSamplerParameter()
{}

void IMaterialGraphBlockSamplerParameter::buildDefaultSockets(graph::BlockLayoutBuilder& builder) const
{
    builder.socket("UV"_id, MaterialInputSocket().hiddenByDefault());
    builder.socket("LodBias"_id, MaterialInputSocket().hiddenByDefault());
    builder.socket("UVRotation"_id, MaterialInputSocket().hiddenByDefault());
}

CodeChunk IMaterialGraphBlockSamplerParameter::compileTextureRef(MaterialStageCompiler& compiler) const
{
    if (const auto* entry = compiler.findParamEntry(parameterName()))
    {
        if (entry->type == MaterialDataLayoutParameterType::Texture2D)
        {
            if (compiler.context().bindlessTextures)
            {

            }
            else
            {
                const auto& layout = compiler.dataLayout()->discreteDataLayout();
                return CodeChunk(CodeChunkType::TextureResource, TempString("{}.{}", layout.descriptorName, entry->name), true);
            }
        }
    }

    return CodeChunk();
}

CodeChunk IMaterialGraphBlockSamplerParameter::computeDefaultUV(MaterialStageCompiler& compiler) const
{
    CodeChunk uv;
    if (hasConnectionOnSocket("UV"_id))
        uv = compiler.evalInput(this, "UV"_id, CodeChunk(Vector2(0, 0))).conform(2);
    else
        uv = compiler.vertexData(MaterialVertexDataType::VertexUV0);

    if (hasConnectionOnSocket("UVRotation"_id))
    {
        const auto y = compiler.evalInput(this, "UVRotation"_id, 0.0f).conform(1);
        const auto x = 1.0f - y.abs();
        const auto baseU = Float2(x, y);
        const auto baseV = Float2(-y, x);
        uv = Float2(Dot2(baseU, uv), Dot2(baseV, uv));
    }

    return uv;
}

///---

class MaterialGraphBlock_SamplerTexture : public IMaterialGraphBlockSamplerParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_SamplerTexture, IMaterialGraphBlockSamplerParameter);

public:
    MaterialGraphBlock_SamplerTexture()
    {}

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Texture2D;
    }

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("RGBA"_id, MaterialOutputSocket().hiddenByDefault());
        builder.socket("RGB"_id, MaterialOutputSocket().swizzle("xyz"_id));
        builder.socket("R"_id, MaterialOutputSocket().color(COLOR_SOCKET_RED).swizzle("x"_id).hiddenByDefault());
        builder.socket("G"_id, MaterialOutputSocket().color(COLOR_SOCKET_GREEN).swizzle("y"_id).hiddenByDefault());
        builder.socket("B"_id, MaterialOutputSocket().color(COLOR_SOCKET_BLUE).swizzle("z"_id).hiddenByDefault());
        builder.socket("A"_id, MaterialOutputSocket().color(COLOR_SOCKET_ALPHA).swizzle("w"_id));

        buildDefaultSockets(builder);
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk sampler = compileTextureRef(compiler);
        if (sampler.empty())
            return CodeChunk(m_fallbackValue);

        CodeChunk uv = computeDefaultUV(compiler);

        CodeChunk ret;
        if (hasConnectionOnSocket("LodBias"_id))
        {
            const auto bias = compiler.evalInput(this, "LodBias"_id, 0.0f).conform(1);
            ret = compiler.var(CodeChunk(CodeChunkType::Numerical4, TempString("textureBias({}, {}, {})", sampler, uv, bias)));
        }
        else
        {
            ret = compiler.var(CodeChunk(CodeChunkType::Numerical4, TempString("texture({}, {})", sampler, uv)));
        }

        if (compiler.debugCode())
            compiler.appendf("if (Frame.CheckMaterialDebug(MATERIAL_FLAG_DISABLE_TEXTURES)) {} = {};\n", ret, CodeChunk(m_fallbackValue));

        return ret;
    }

private:
    Vector4 m_fallbackValue = Vector4(1,1,1,1);
};

RTTI_BEGIN_TYPE_CLASS(MaterialGraphBlock_SamplerTexture);
    RTTI_METADATA(graph::BlockInfoMetadata).title("Texture Sampler").group("Samplers").name("Sample Texture");
    RTTI_METADATA(graph::BlockTitleColorMetadata).color(COLOR_SOCKET_TEXTURE);
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialSampler");
    RTTI_PROPERTY(m_fallbackValue).editable();
RTTI_END_TYPE();

///---

class MaterialGraphBlock_SamplerNormal : public IMaterialGraphBlockSamplerParameter
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_SamplerNormal, IMaterialGraphBlockSamplerParameter);

public:
    MaterialGraphBlock_SamplerNormal()
    {}

    virtual MaterialDataLayoutParameterType parameterType() const override
    {
        return MaterialDataLayoutParameterType::Texture2D;
    }

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("Normal"_id, MaterialOutputSocket());

        buildDefaultSockets(builder);
    }

    virtual CodeChunk compile(MaterialStageCompiler& compiler, StringID outputName) const override
    {
        CodeChunk sampler = compileTextureRef(compiler);
        if (sampler.empty())
            return CodeChunk(Vector3(0, 0, 1));

        CodeChunk uv = computeDefaultUV(compiler);

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
RTTI_METADATA(graph::BlockInfoMetadata).title("Normals Sampler").group("Samplers").name("Sample Normals");
RTTI_METADATA(graph::BlockTitleColorMetadata).color(COLOR_SOCKET_TEXTURE);
RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialSampler");
RTTI_PROPERTY(m_sampleInTangentSpace).editable("Output normal in tangent space rather than in world space");
RTTI_PROPERTY(m_normalScale).editable("Scale the output normal");
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
