/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingStates.h"

namespace rendering
{

    //---

    RTTI_BEGIN_TYPE_ENUM(FilterMode);
        RTTI_ENUM_OPTION(Nearest);
        RTTI_ENUM_OPTION(Linear);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(MipmapFilterMode);
        RTTI_ENUM_OPTION(Nearest);
        RTTI_ENUM_OPTION(Linear);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(AddressMode);
        RTTI_ENUM_OPTION(Wrap);
        RTTI_ENUM_OPTION(Mirror);
        RTTI_ENUM_OPTION(ClampToEdge);
        RTTI_ENUM_OPTION(ClampToBorder);
        RTTI_ENUM_OPTION(MirrorClampToEdge);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(BorderColor);
        RTTI_ENUM_OPTION(FloatTransparentBlack);
        RTTI_ENUM_OPTION(FloatOpaqueBlack);
        RTTI_ENUM_OPTION(FloatOpaqueWhite);
        RTTI_ENUM_OPTION(IntTransparentBlack);
        RTTI_ENUM_OPTION(IntOpaqueBlack);
        RTTI_ENUM_OPTION(IntOpaqueWhite);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(CompareOp);
        RTTI_ENUM_OPTION(Never);
        RTTI_ENUM_OPTION(Less);
        RTTI_ENUM_OPTION(LessEqual);
        RTTI_ENUM_OPTION(Greater);
        RTTI_ENUM_OPTION(Equal);
        RTTI_ENUM_OPTION(NotEqual);
        RTTI_ENUM_OPTION(GreaterEqual);
        RTTI_ENUM_OPTION(Always);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(LogicalOp);
        RTTI_ENUM_OPTION(Clear);
        RTTI_ENUM_OPTION(And);
        RTTI_ENUM_OPTION(AndReverse);
        RTTI_ENUM_OPTION(Copy);
        RTTI_ENUM_OPTION(AndInverted);
        RTTI_ENUM_OPTION(NoOp);
        RTTI_ENUM_OPTION(Xor);
        RTTI_ENUM_OPTION(Or);
        RTTI_ENUM_OPTION(Nor);
        RTTI_ENUM_OPTION(Equivalent);
        RTTI_ENUM_OPTION(Invert);
        RTTI_ENUM_OPTION(OrReverse);
        RTTI_ENUM_OPTION(CopyInverted);
        RTTI_ENUM_OPTION(OrInverted);
        RTTI_ENUM_OPTION(Nand);
        RTTI_ENUM_OPTION(Set);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(BlendFactor);
        RTTI_ENUM_OPTION(Zero);
        RTTI_ENUM_OPTION(One);
        RTTI_ENUM_OPTION(SrcColor);
        RTTI_ENUM_OPTION(OneMinusSrcColor);
        RTTI_ENUM_OPTION(DestColor);
        RTTI_ENUM_OPTION(OneMinusDestColor);
        RTTI_ENUM_OPTION(SrcAlpha);
        RTTI_ENUM_OPTION(OneMinusSrcAlpha);
        RTTI_ENUM_OPTION(DestAlpha);
        RTTI_ENUM_OPTION(OneMinusDestAlpha);
        RTTI_ENUM_OPTION(ConstantColor);
        RTTI_ENUM_OPTION(OneMinusConstantColor);
        RTTI_ENUM_OPTION(ConstantAlpha);
        RTTI_ENUM_OPTION(OneMinusConstantAlpha);
        RTTI_ENUM_OPTION(SrcAlphaSaturate);
        RTTI_ENUM_OPTION(Src1Color);
        RTTI_ENUM_OPTION(OneMinusSrc1Color);
        RTTI_ENUM_OPTION(Src1Alpha);
        RTTI_ENUM_OPTION(OneMinusSrc1Alpha);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(BlendOp);
        RTTI_ENUM_OPTION(Add);
        RTTI_ENUM_OPTION(Subtract);
        RTTI_ENUM_OPTION(ReverseSubtract);
        RTTI_ENUM_OPTION(Min);
        RTTI_ENUM_OPTION(Max);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(StencilOp);
        RTTI_ENUM_OPTION(Keep);
        RTTI_ENUM_OPTION(Zero);
        RTTI_ENUM_OPTION(Replace);
        RTTI_ENUM_OPTION(IncrementAndClamp);
        RTTI_ENUM_OPTION(DecrementAndClamp);
        RTTI_ENUM_OPTION(Invert);
        RTTI_ENUM_OPTION(IncrementAndWrap);
        RTTI_ENUM_OPTION(DecrementAndWrap);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(PolygonMode);
        RTTI_ENUM_OPTION(Fill);
        RTTI_ENUM_OPTION(Line);
        RTTI_ENUM_OPTION(Point);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(CullMode);
        RTTI_ENUM_OPTION(Disabled);
        RTTI_ENUM_OPTION(Front);
        RTTI_ENUM_OPTION(Back);
        RTTI_ENUM_OPTION(Both);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(FrontFace);
        RTTI_ENUM_OPTION(CCW);
        RTTI_ENUM_OPTION(CW);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(PrimitiveTopology);
        RTTI_ENUM_OPTION(PointList);
        RTTI_ENUM_OPTION(LineList);
        RTTI_ENUM_OPTION(LineStrip);
        RTTI_ENUM_OPTION(TriangleList);
        RTTI_ENUM_OPTION(TriangleStrip);
        RTTI_ENUM_OPTION(TriangleFan);
        RTTI_ENUM_OPTION(LineListWithAdjacency);
        RTTI_ENUM_OPTION(LineStripWithAdjacency);
        RTTI_ENUM_OPTION(TriangleListWithAdjacency);
        RTTI_ENUM_OPTION(TriangleStripWithAdjacency);
        RTTI_ENUM_OPTION(PatchList);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(LoadOp);
        RTTI_ENUM_OPTION(Keep);
        RTTI_ENUM_OPTION(Clear);
        RTTI_ENUM_OPTION(DontCare);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(StoreOp);
        RTTI_ENUM_OPTION(Store);
        RTTI_ENUM_OPTION(DontCare);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(ShaderType);
        RTTI_ENUM_OPTION(Pixel);
        RTTI_ENUM_OPTION(Vertex);
        RTTI_ENUM_OPTION(Geometry);
        RTTI_ENUM_OPTION(Hull);
        RTTI_ENUM_OPTION(Domain);
        RTTI_ENUM_OPTION(Compute);
    RTTI_END_TYPE()

    RTTI_BEGIN_TYPE_ENUM(Stage); // for bariers
        RTTI_ENUM_OPTION(All);
        RTTI_ENUM_OPTION(VertexInput);
        RTTI_ENUM_OPTION(VertexShaderInput);
        RTTI_ENUM_OPTION(PixelShaderInput);
        RTTI_ENUM_OPTION(ComputeShaderInput);
        RTTI_ENUM_OPTION(RasterInput);
        RTTI_ENUM_OPTION(ShaderOutput);
        RTTI_ENUM_OPTION(RasterOutput);
    RTTI_END_TYPE()

    //---

    uint32_t SamplerState::CalcHash(const SamplerState& key)
    {
        base::CRC64 crc;
        crc << (uint8_t)key.magFilter;
        crc << (uint8_t)key.minFilter;
        crc << (uint8_t)key.mipmapMode;
        crc << (uint8_t)key.addresModeU;
        crc << (uint8_t)key.addresModeV;
        crc << (uint8_t)key.addresModeW;
        crc << key.mipLodBias.value;
        crc << key.maxAnisotropy;
        crc << key.compareEnabled;
        if (key.compareEnabled)
            crc << (uint8_t)key.compareOp;
        crc << key.minLod.value;
        crc << key.maxLod.value;
        crc << (uint8_t)key.borderColor;
        return crc.crc();
    }

    bool SamplerState::operator==(const SamplerState& other) const
    {
        return (magFilter == other.magFilter) && (minFilter == other.minFilter) && (mipmapMode == other.mipmapMode)
            && (addresModeU == other.addresModeU) && (addresModeV == other.addresModeV) && (addresModeW == other.addresModeW)
            && (compareOp == other.compareOp) && (borderColor == other.borderColor)
            && (mipLodBias.value == other.mipLodBias.value) && (minLod.value == other.minLod.value) && (maxLod.value == other.maxLod.value)
            && (maxAnisotropy == other.maxAnisotropy) && (compareEnabled == other.compareEnabled);
    }

    bool SamplerState::operator!=(const SamplerState& other) const
    {
        return !operator==(other);
    }

    //--

    uint32_t BlendState::CalcHash(const BlendState& key)
    {
        base::CRC32 crc;
        crc << (uint8_t)key.srcColorBlendFactor;
        crc << (uint8_t)key.destColorBlendFactor;
        crc << (uint8_t)key.colorBlendOp;
        crc << (uint8_t)key.srcAlphaBlendFactor;
        crc << (uint8_t)key.destAlphaBlendFactor;
        crc << (uint8_t)key.alphaBlendOp;
        return crc;
    }

    bool BlendState::operator==(const BlendState& other) const
    {
        if ((srcColorBlendFactor != other.srcColorBlendFactor) || (destColorBlendFactor != other.destColorBlendFactor) || (colorBlendOp != other.colorBlendOp))
            return false;

        if ((srcAlphaBlendFactor != other.srcAlphaBlendFactor) || (destAlphaBlendFactor != other.destAlphaBlendFactor) || (alphaBlendOp != other.alphaBlendOp))
            return false;

        return true;
    }

    bool BlendState::operator!=(const BlendState& other) const
    {
        return !operator==(other);
    }

    //--

    uint32_t StencilSideState::CalcHash(const StencilSideState& key)
    {
        base::CRC32 crc;
        crc << (uint8_t)key.failOp;
        crc << (uint8_t)key.passOp;
        crc << (uint8_t)key.depthFailOp;
        crc << (uint8_t)key.compareOp;
        crc << key.compareMask;
        crc << key.writeMask;
        crc << key.referenceValue;
        return crc;
    }

    bool StencilSideState::operator==(const StencilSideState& other) const
    {
        return (failOp == other.failOp) && (passOp == other.passOp) && (depthFailOp == other.depthFailOp) && (compareOp == other.compareOp)
            && (compareMask == other.compareMask) && (writeMask == other.writeMask) && (referenceValue == other.referenceValue);
    }

    bool StencilSideState::operator!=(const StencilSideState& other) const
    {
        return !operator==(other);
    }

    //---

    uint32_t StencilState::CalcHash(const StencilState& key)
    {
        base::CRC32 crc;
        crc << key.enabled;
        if (key.enabled)
        {
            crc << StencilSideState::CalcHash(key.front);
            crc << StencilSideState::CalcHash(key.back);
        }
        return crc;
    }

    bool StencilState::operator==(const StencilState& other) const
    {
        if (enabled != other.enabled)
            return false;

        if (enabled)
        {
            if (front != other.front || back != other.back)
                return false;
        }

        return true;
    }

    bool StencilState::operator!=(const StencilState& other) const
    {
        return !operator==(other);
    }

    //---

    uint32_t CullState::CalcHash(const CullState& key)
    {
        base::CRC32 crc;
        crc << (uint8_t)key.mode;
        crc << (uint8_t)key.face;
        return crc;
    }

    bool CullState::operator==(const CullState& other) const
    {
        return (mode == other.mode) && (face == other.face);
    }

    bool CullState::operator!=(const CullState& other) const
    {
        return !operator==(other);
    }

    //---

    uint32_t FillState::CalcHash(const FillState& key)
    {
        base::CRC32 crc;
        crc << (uint8_t)key.mode;
        crc << key.lineWidth;
        return crc;
    }

    bool FillState::operator==(const FillState& other) const
    {
        return (mode == other.mode) && (lineWidth == other.lineWidth);
    }

    bool FillState::operator!=(const FillState& other) const
    {
        return !operator==(other);
    }

    //----

    uint32_t DepthState::CalcHash(const DepthState& key)
    {
        base::CRC32 crc;
        crc << key.enabled;
        if (key.enabled)
        {
            crc << key.writeEnabled;
            crc << (uint8_t)key.depthCompareOp;
        }
        return crc;
    }

    bool DepthState::operator==(const DepthState& other) const
    {
        if (enabled != other.enabled)
            return false;
        if (enabled)
        {
            if (writeEnabled != other.writeEnabled || depthCompareOp != other.depthCompareOp)
                return false;
        }
        return true;
    }

    bool DepthState::operator!=(const DepthState& other) const
    {
        return !operator==(other);
    }

    //----

    uint32_t DepthClipState::CalcHash(const DepthClipState& key)
    {
        base::CRC32 crc;
        crc << key.enabled;
        if (key.enabled)
        {
            crc << key.clipMin;
            crc << key.clipMax;
        }
        return crc;
    }

    bool DepthClipState::operator==(const DepthClipState& other) const
    {
        if (enabled != other.enabled)
            return false;

        if (enabled)
            if ((clipMax != other.clipMin) || (clipMax != other.clipMax))
                return false;

        return true;
    }

    bool DepthClipState::operator!=(const DepthClipState& other) const
    {
        return !operator==(other);
    }

    //----

    uint32_t DepthBiasState::CalcHash(const DepthBiasState& key)
    {
        base::CRC32 crc;
        crc << key.enabled;
        if (key.enabled)
        {
            crc << key.constant;
            crc << key.slope;
            crc << key.clamp;
        }
        return crc;
    }


    bool DepthBiasState::operator==(const DepthBiasState& other) const
    {
        if (enabled != other.enabled)
            return false;

        if (enabled)
        {
            if (constant != other.constant || slope != other.slope || clamp != other.clamp)
                return false;
        }

        return true;
    }

    bool DepthBiasState::operator!=(const DepthBiasState& other) const
    {
        return !operator==(other);
    }

    //----

    uint32_t PrimitiveAssemblyState::CalcHash(const PrimitiveAssemblyState& key)
    {
        base::CRC32 crc;
        crc << (uint8_t)key.topology;
        crc << key.restartEnabled;
        return crc;
    }

    bool PrimitiveAssemblyState::operator==(const PrimitiveAssemblyState& other) const
    {
        return (topology == other.topology) && (restartEnabled && other.restartEnabled);
    }

    bool PrimitiveAssemblyState::operator!=(const PrimitiveAssemblyState& other) const
    {
        return !operator==(other);
    }

    //----

    uint32_t MultisampleState::CalcHash(const MultisampleState& key)
    {
        base::CRC32 crc;
        crc << key.sampleCount;
        crc << key.sampleShadingEnable;
        crc << key.alphaToCoverageEnable;
        crc << key.alphaToOneEnable;
        crc << key.minSampleShading;
        crc << key.sampleMask;
        return crc;
    }

    bool MultisampleState::operator==(const MultisampleState& other) const
    {
        return (sampleCount == other.sampleCount) && (sampleShadingEnable == other.sampleShadingEnable)
            && (alphaToCoverageEnable == other.alphaToCoverageEnable) && (alphaToOneEnable == other.alphaToOneEnable)
            && (minSampleShading == other.minSampleShading) && (sampleMask == other.sampleMask);
    }

    bool MultisampleState::operator!=(const MultisampleState& other) const
    {
        return !operator==(other);
    }

    //----

} // rendering