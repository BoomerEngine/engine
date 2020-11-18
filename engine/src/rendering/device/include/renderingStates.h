/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

namespace rendering
{
    //---
    /// ENUMS, LOTS OF ENUMS
    ///---

    enum class FilterMode : uint8_t
    {
        Nearest, // point filtering
        Linear,
    };

    enum class MipmapFilterMode : uint8_t
    {
        None,
        Nearest,
        Linear,
    };

    enum class AddressMode : uint8_t
    {
        Wrap,
        Mirror,
        ClampToEdge,
        ClampToBorder,
        MirrorClampToEdge,
    };

    enum class BorderColor : uint8_t
    {
        FloatTransparentBlack,
        FloatOpaqueBlack,
        FloatOpaqueWhite,
        IntTransparentBlack,
        IntOpaqueBlack,
        IntOpaqueWhite,
    };

#undef Always // X11 FFS
    enum class CompareOp : uint8_t
    {
        Never,
        Less,
        LessEqual,
        Greater,
        Equal,
        NotEqual,
        GreaterEqual,
        Always,
    };

    enum class LogicalOp : uint8_t
    {
        Clear,
        And,
        AndReverse,
        Copy,
        AndInverted,
        NoOp,
        Xor,
        Or,
        Nor,
        Equivalent,
        Invert,
        OrReverse,
        CopyInverted,
        OrInverted,
        Nand,
        Set,
    };

    enum class BlendFactor : uint8_t
    {
        Zero,
        One,
        SrcColor,
        OneMinusSrcColor,
        DestColor,
        OneMinusDestColor,
        SrcAlpha,
        OneMinusSrcAlpha,
        DestAlpha,
        OneMinusDestAlpha,
        ConstantColor,
        OneMinusConstantColor,
        ConstantAlpha,
        OneMinusConstantAlpha,
        SrcAlphaSaturate,
        Src1Color,
        OneMinusSrc1Color,
        Src1Alpha,
        OneMinusSrc1Alpha,
    };

    enum class BlendOp : uint8_t
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };

    enum class StencilOp : uint8_t
    {
        Keep,
        Zero,
        Replace,
        IncrementAndClamp,
        DecrementAndClamp,
        Invert,
        IncrementAndWrap,
        DecrementAndWrap,
    };

    enum class PolygonMode : uint8_t
    {
        Fill,
        Line,
        Point,
    };

    enum class CullMode : uint8_t
    {
        Disabled,
        Front,
        Back,
        Both,
    };

    enum class FrontFace : uint8_t
    {
        CCW,
        CW,
    };

    enum class PrimitiveTopology : uint8_t
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        TriangleFan,

        // magic part:
        LineListWithAdjacency,
        LineStripWithAdjacency,
        TriangleListWithAdjacency,
        TriangleStripWithAdjacency,
        PatchList,
    };

    enum class LoadOp : uint8_t
    {
        Keep, // keep data that exists in the memory
        Clear, // clear the attachment when binding
        DontCare, // we don't care (fastest, can produce artifacts if all pixels are not written)
    };

    enum class StoreOp : uint8_t
    {
        Store, // store the data into the memory
        DontCare, // we don't care
    };

    enum class ShaderType : uint8_t
    {
        // NOTE: DO NOT CHANGE ORDER - this determines shader dependencies used in the engine
        None,
        Pixel,
        Geometry,
        Domain,
        Hull,
        Vertex,
        Compute,
        MAX,
    };

    enum class Stage : uint8_t // for barriers
    {
        All = 0, // consider all stages a dependency, most wide, default one since it's hard to come up with better narrowing
        VertexInput, // stuff will be needed before vertex input starts
        VertexShaderInput, // stuff will be needed before vertex shader start
        PixelShaderInput, // stuff will be needed before pixel shader start
        ComputeShaderInput, // stuff will be needed before compute shader start
        RasterInput, // stuff will be needed before rasterization starts
        ShaderOutput, // stuff is produced by the shaders
        RasterOutput, // stuff is produced by rasterization
    };

    enum class ResourceType : uint8_t
    {
        None,
        Constants, // UOB/ConstantBuffer
        Texture, // strict texture with sampler, read only, format provided
        Buffer, // typed buffer, format or layout must be provided, CAN be writable/readable

        // place more resource types
    };

    enum class ResourceAccess : uint8_t
    {
        ReadOnly,
        UAVReadWrite,
        UAVReadOnly,
        UAVWriteOnly,
    };

    ///---
    /// DATA STRUCTURES DESCRIBING RENDER STATES
    ///---

#pragma pack(push)
#pragma pack(1)

    /// blend state for single render target, updated via opSetBlendState
    struct RENDERING_DEVICE_API BlendState
    {
        BlendFactor srcColorBlendFactor = BlendFactor::One;
        BlendFactor destColorBlendFactor = BlendFactor::Zero;
        BlendOp colorBlendOp = BlendOp::Add;
        BlendFactor srcAlphaBlendFactor = BlendFactor::One;
        BlendFactor destAlphaBlendFactor = BlendFactor::Zero;
        BlendOp alphaBlendOp = BlendOp::Add;

        static uint32_t CalcHash(const BlendState& key);

        bool operator==(const BlendState& other) const;
        bool operator!=(const BlendState& other) const;
    };

    ///---

    /// stencil operation state for a single facing
    struct RENDERING_DEVICE_API StencilSideState
    {
        StencilOp failOp = StencilOp::Keep;
        StencilOp passOp = StencilOp::Keep;
        StencilOp depthFailOp = StencilOp::Keep;
        CompareOp compareOp = CompareOp::Always;
        uint8_t compareMask = 0xFF;
        uint8_t writeMask = 0xFF;
        uint8_t referenceValue = 0xFF;

        static uint32_t CalcHash(const StencilSideState& key);

        bool operator==(const StencilSideState& other) const;
        bool operator!=(const StencilSideState& other) const;
    };

    /// stencil op state
    struct RENDERING_DEVICE_API StencilState
    {
        bool enabled = 0;
        StencilSideState front;
        StencilSideState back;

        static uint32_t CalcHash(const StencilState& key);

        bool operator==(const StencilState& other) const;
        bool operator!=(const StencilState& other) const;
    };

    ///---

    /// culling state
    struct RENDERING_DEVICE_API CullState
    {
        CullMode mode = CullMode::Back;
        FrontFace face = FrontFace::CW;

        static uint32_t CalcHash(const CullState& key);

        bool operator==(const CullState& other) const;
        bool operator!=(const CullState& other) const;
    };

    ///---

    /// polygon rasterization state
    struct RENDERING_DEVICE_API FillState
    {
        PolygonMode mode = PolygonMode::Fill;
        float lineWidth = 1.0f;

        static uint32_t CalcHash(const FillState& key);

        bool operator==(const FillState& other) const;
        bool operator!=(const FillState& other) const;
    };

    ///---

    /// depth testing state
    struct RENDERING_DEVICE_API DepthState
    {
        bool enabled = 0;
        bool writeEnabled = 0;
        CompareOp depthCompareOp = CompareOp::Always;

        static uint32_t CalcHash(const DepthState& key);

        bool operator==(const DepthState& other) const;
        bool operator!=(const DepthState& other) const;
    };

    /// depth clip state
    struct RENDERING_DEVICE_API DepthClipState
    {
        bool enabled = false;
        float clipMin = 0.0f;
        float clipMax = 1.0f;

        static uint32_t CalcHash(const DepthClipState& key);

        bool operator==(const DepthClipState& other) const;
        bool operator!=(const DepthClipState& other) const;
    };

    /// depth bias state
    struct RENDERING_DEVICE_API DepthBiasState
    {
        bool enabled = 0;
        float constant = 0.0f;
        float slope = 0.0f;
        float clamp = 0.0f;

        static uint32_t CalcHash(const DepthBiasState& key);

        bool operator==(const DepthBiasState& other) const;
        bool operator!=(const DepthBiasState& other) const;
    };

    ///----

    /// primitive assembly state
    struct RENDERING_DEVICE_API PrimitiveAssemblyState
    {
        PrimitiveTopology topology = PrimitiveTopology::TriangleList;
        bool restartEnabled = 0;

        static uint32_t CalcHash(const PrimitiveAssemblyState& key);

        bool operator==(const PrimitiveAssemblyState& other) const;
        bool operator!=(const PrimitiveAssemblyState& other) const;
    };

    ///---

    /// multisampling state
    struct RENDERING_DEVICE_API MultisampleState
    {
        uint8_t sampleCount = 1;
        bool sampleShadingEnable = 0;
        bool alphaToCoverageEnable = 0;
        bool alphaToCoverageDitherEnable = 0;
        bool  alphaToOneEnable = 0;
        float minSampleShading = 0.0f;
        uint32_t sampleMask = 0xFFFFFFFF;

        static uint32_t CalcHash(const MultisampleState& key);

        bool operator==(const MultisampleState& other) const;
        bool operator!=(const MultisampleState& other) const;
    };

    /// quantized value for use with samplers
    /// sign + 11.4 fixed point 
    struct SamplerQuantizedLODValue
    {
        short value = 0;

        INLINE SamplerQuantizedLODValue() {};

        INLINE SamplerQuantizedLODValue(float fullRangeValue)
        {
            float clampedValue = std::clamp(fullRangeValue, -64.0f, 64.0f);
            value = (short)(clampedValue * 16.0f);
        }

        INLINE operator float() const
        {
            return value * 0.0625f;
        }
    };

    /// sampler state
    struct RENDERING_DEVICE_API SamplerState
    {
        FilterMode magFilter = FilterMode::Nearest;
        FilterMode minFilter = FilterMode::Nearest;
        MipmapFilterMode mipmapMode = MipmapFilterMode::Nearest;
        AddressMode addresModeU = AddressMode::ClampToEdge;
        AddressMode addresModeV = AddressMode::ClampToEdge;
        AddressMode addresModeW = AddressMode::ClampToEdge;
        CompareOp compareOp = CompareOp::LessEqual;
        BorderColor borderColor = BorderColor::FloatTransparentBlack;
        SamplerQuantizedLODValue mipLodBias = 0.0f;
        SamplerQuantizedLODValue minLod = 0.0f;
        SamplerQuantizedLODValue maxLod = 16.0f;
        uint8_t maxAnisotropy = 0; // unlimited
        bool compareEnabled = false;

        static uint32_t CalcHash(const SamplerState& key);

        bool operator==(const SamplerState& other) const;
        bool operator!=(const SamplerState& other) const;
    };

    //---

#pragma pack(pop)

} // rendering
