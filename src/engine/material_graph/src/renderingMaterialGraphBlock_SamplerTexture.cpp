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

class MaterialGraphBlock_SamplerTexture : public MaterialGraphBlock
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialGraphBlock_SamplerTexture, MaterialGraphBlock);

public:
    MaterialGraphBlock_SamplerTexture()
    {}

    virtual void buildLayout(graph::BlockLayoutBuilder& builder) const override
    {
        builder.socket("RGB"_id, MaterialOutputSocket().swizzle("xyz"_id));
        builder.socket("R"_id, MaterialOutputSocket().color(COLOR_SOCKET_RED).swizzle("x"_id).hiddenByDefault());
        builder.socket("G"_id, MaterialOutputSocket().color(COLOR_SOCKET_GREEN).swizzle("y"_id).hiddenByDefault());
        builder.socket("B"_id, MaterialOutputSocket().color(COLOR_SOCKET_BLUE).swizzle("z"_id).hiddenByDefault());
        builder.socket("A"_id, MaterialOutputSocket().color(COLOR_SOCKET_ALPHA).swizzle("w"_id));

        builder.socket("Texture"_id, MaterialInputSocket().tag("TEXTURE"_id).color(COLOR_SOCKET_TEXTURE));

        builder.socket("UV"_id, MaterialInputSocket());
        builder.socket("LodBias"_id, MaterialInputSocket());
        builder.socket("UVRotation"_id, MaterialInputSocket());
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

        if (hasConnectionOnSocket("UVRotation"_id))
        {
            const auto y = compiler.evalInput(this, "UVRotation"_id, 0.0f).conform(1);
            const auto x = 1.0f - y.abs();
            const auto baseU = Float2(x, y);
            const auto baseV = Float2(-y, x);
            uv = Float2(Dot2(baseU, uv), Dot2(baseV, uv));
        }

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
    RTTI_METADATA(graph::BlockInfoMetadata).title("SamplerTexture").group("Samplers");
    RTTI_METADATA(graph::BlockTitleColorMetadata).color(COLOR_SOCKET_TEXTURE);
    RTTI_METADATA(graph::BlockStyleNameMetadata).style("MaterialSampler");
    RTTI_PROPERTY(m_fallbackValue).editable();
RTTI_END_TYPE();

///---

END_BOOMER_NAMESPACE()
