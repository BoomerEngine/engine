/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#pragma once

#include "renderingImageFormat.h"

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
	#define COMPARE_OP_BITS 3
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
	static_assert((int)CompareOp::Always < (1U << COMPARE_OP_BITS), "Adjust COMPARE_OP_BITS");

	#define LOGICAL_OP_BITS 4
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
	static_assert((int)LogicalOp::Set < (1U << LOGICAL_OP_BITS), "Adjust LOGICAL_OP_BITS");

	#define BLEND_FACTOR_BITS 5
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

		MAX,
    };
	static_assert((int)BlendFactor::OneMinusSrc1Alpha < (1U << BLEND_FACTOR_BITS), "Adjust BLEND_FACTOR_BITS");

	#define BLEND_OP_BITS 3
    enum class BlendOp : uint8_t
    {
        Add,
        Subtract,
        ReverseSubtract,
        Min,
        Max,
    };
	static_assert((int)BlendOp::Max < (1U << BLEND_OP_BITS), "Adjust BLEND_OP_BITS");

	#define STENCIL_OP_BITS 3
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
	static_assert((int)StencilOp::DecrementAndWrap < (1U << STENCIL_OP_BITS), "Adjust STENCIL_OP_BITS");

	#define FILL_MODE_BITS 2
    enum class FillMode : uint8_t
    {
        Fill,
        Line,
        Point,
    };
	static_assert((int)FillMode::Point < (1U << FILL_MODE_BITS), "Adjust FILL_MODE_BITS");

	#define CULL_MODE_BITS 2
    enum class CullMode : uint8_t
    {
        Disabled,
        Front,
        Back,
        Both,
    };
	static_assert((int)CullMode::Both < (1U << CULL_MODE_BITS), "Adjust CULL_MODE_BITS");

	#define FRONT_FACE_MODE_BITS 2
    enum class FrontFace : uint8_t
    {
        CCW,
        CW,
    };
	static_assert((int)FrontFace::CW < (1U << FRONT_FACE_MODE_BITS), "Adjust FRONT_FACE_MODE_BITS");

	#define PRIMITIVE_TOPOLOGY_BITS 4
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
	static_assert((int)PrimitiveTopology::PatchList < (1U << PRIMITIVE_TOPOLOGY_BITS), "Adjust PRIMITIVE_TOPOLOGY_BITS");

	//--

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

	typedef base::BitFlags<ShaderType> ShaderTypeMask;

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

	//--

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

		base::StringBuf label;

        static uint32_t CalcHash(const SamplerState& key);

        bool operator==(const SamplerState& other) const;
        bool operator!=(const SamplerState& other) const;
    };

	//---

	// bit describing which render state is dirty
	enum class StaticRenderStateDirtyBit : uint64_t
	{
		FillMode,

		CullEnabled,
		CullMode,
		CullFrontFace,

		DepthEnabled,
		DepthWriteEnabled,
		DepthBiasEnabled,
		DepthBoundsEnabled,
		DepthFunc,

		ScissorEnabled,

		StencilEnabled,
		StencilFrontOps,
		StencilBackOps,

		PrimitiveRestartEnabled,
		PrimitiveTopology,

		BlendingEnabled,

		AlphaCoverageEnabled,
		AlphaCoverageDitherEnabled,

		//--

		BlendEquation0,
		BlendEquation1,
		BlendEquation2,
		BlendEquation3,
		BlendEquation4,
		BlendEquation5,
		BlendEquation6,
		BlendEquation7,

		BlendFunc0,
		BlendFunc1,
		BlendFunc2,
		BlendFunc3,
		BlendFunc4,
		BlendFunc5,
		BlendFunc6,
		BlendFunc7,

		ColorMask0,
		ColorMask1,
		ColorMask2,
		ColorMask3,
		ColorMask4,
		ColorMask5,
		ColorMask6,
		ColorMask7,

		MAX,
	};

	static_assert((int)StaticRenderStateDirtyBit::MAX <= 64, "To many render states to track");

	typedef base::BitFlags<StaticRenderStateDirtyBit> StaticRenderStateDirtyFlags;

	//--

	/// helper structure for collecting (setting) all STATIC render states that are usually baked in
#pragma pack(push)
#pragma pack(1)
	struct RENDERING_DEVICE_API StaticRenderStatesSetup
	{
		static const uint8_t MAX_TARGETS = 8;

		static const StaticRenderStateDirtyBit COLOR_MASK_DIRTY_BITS[StaticRenderStatesSetup::MAX_TARGETS];
		static const StaticRenderStateDirtyBit BLEND_FUNC_DIRTY_BITS[StaticRenderStatesSetup::MAX_TARGETS];
		static const StaticRenderStateDirtyBit BLEND_EQUATION_DIRTY_BITS[StaticRenderStatesSetup::MAX_TARGETS];

		struct BlendState
		{
			uint32_t srcColorBlendFactor : BLEND_FACTOR_BITS;
			uint32_t destColorBlendFactor : BLEND_FACTOR_BITS;
			uint32_t colorBlendOp : BLEND_OP_BITS;
			uint32_t srcAlphaBlendFactor : BLEND_FACTOR_BITS;
			uint32_t destAlphaBlendFactor : BLEND_FACTOR_BITS;
			uint32_t alphaBlendOp : BLEND_OP_BITS;
		};
		static_assert(sizeof(BlendState) == 4, "Keep small");

		struct CommonState
		{
			uint64_t fillMode : FILL_MODE_BITS; // PolygonMode
			uint64_t depthCompareOp : COMPARE_OP_BITS; // CompareOp
			uint64_t depthEnabled : 1;
			uint64_t depthWriteEnabled : 1;
			uint64_t depthBiasEnabled : 1;
			uint64_t depthClipEnabled : 1;

			uint64_t stencilEnabled : 1;
			uint64_t stencilFrontFailOp : STENCIL_OP_BITS;
			uint64_t stencilFrontPassOp : STENCIL_OP_BITS;
			uint64_t stencilFrontDepthFailOp : STENCIL_OP_BITS;
			uint64_t stencilFrontCompareOp : COMPARE_OP_BITS;
			uint64_t stencilBackFailOp : STENCIL_OP_BITS;
			uint64_t stencilBackPassOp : STENCIL_OP_BITS;
			uint64_t stencilBackDepthFailOp : STENCIL_OP_BITS;
			uint64_t stencilBackCompareOp : COMPARE_OP_BITS;

			uint64_t cullEnabled : 1;
			uint64_t cullMode : CULL_MODE_BITS; // CullMode
			uint64_t cullFrontFace : 1; // FrontFace 

			uint64_t primitiveTopology : PRIMITIVE_TOPOLOGY_BITS; // PrimitiveTopology;
			uint64_t primitiveRestartEnabled : 1;

			uint64_t multisampleSampleCount : 4;
			uint64_t multisamplePerSameShadingEnable : 1;
			uint64_t alphaToCoverageEnable : 1;
			uint64_t alphaToCoverageDitherEnable : 1;
			uint64_t alphaToOneEnable : 1;

			uint64_t scissorEnabled : 1;
			uint64_t blendingEnabled : 1;
		};		
		static_assert(sizeof(CommonState) == 8, "Keep small");

		CommonState common; 
		BlendState blendStates[MAX_TARGETS];
		uint8_t colorMasks[MAX_TARGETS];

		//--

		static StaticRenderStatesSetup& DEFAULT_STATES();

		//--

		StaticRenderStatesSetup();

		//--

		uint64_t key() const;

		void apply(const StaticRenderStatesSetup& other, const StaticRenderStateDirtyFlags* mask = nullptr);

		void print(base::IFormatStream& f) const;

		void print(base::IFormatStream& f, const StaticRenderStateDirtyFlags& stateMask) const;

		bool operator==(const StaticRenderStatesSetup& other) const;
		INLINE bool operator!=(const StaticRenderStatesSetup& other) const { return !operator==(other); }

		//--
	};
#pragma pack(pop)

	/// helper structure for collecting (setting) all render states
	class StaticRenderStatesBuilder
	{
		RTTI_DECLARE_POOL(POOL_RENDERING_RUNTIME);

	public:
		StaticRenderStatesBuilder();

		//--

		StaticRenderStatesSetup states;
		StaticRenderStateDirtyFlags dirtyFlags;

		//--

		/// reset to default states
		void reset();

		/// set color mask
		void colorMask(uint8_t rtIndex, uint8_t mask);

		/// set blend state for a given render target
		void blend(bool enabled);
		void blendFactor(uint8_t renderTargetIndex, BlendFactor src, BlendFactor dest);
		void blendFactor(uint8_t renderTargetIndex, BlendFactor srcColor, BlendFactor destColor, BlendFactor srcAlpha, BlendFactor destAlpha);
		void blendOp(uint8_t renderTargetIndex, BlendOp op);
		void blendOp(uint8_t renderTargetIndex, BlendOp color, BlendOp alpha);

		/// set cull state
		void cull(bool enabled);
		void cullMode(CullMode mode);
		void cullFrontFace(FrontFace mode);

		/// set polygon mode
		void fill(FillMode mode);

		/// enable/disable scissoring
		void scissorState(bool enabled);

		/// set the stencil state
		void stencil(bool enabled); // disable
		void stencilAll(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp);
		void stencilFront(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp);
		void stencilBack(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp);

		/// set depth state
		void depth(bool enable);
		void depthWrite(bool enable);
		void depthFunc(CompareOp func);
		void depthClip(bool enabled);
		void depthBias(bool enabled);

		/// set primitive assembly
		void primitiveTopology(PrimitiveTopology topology);
		void primitiveRestart(bool enabled);

		/// set multisample state
		//void multisample(uint8_t numSamples);

		/// alpha to coverage
		void alphaToCoverage(bool enabled);
		void alphaToCoverageDither(bool enabled);
		void alphaToOne(bool enabled);
	};

	//--

#pragma pack(push)
#pragma pack(1)
	/// pass attachment state
	struct RENDERING_DEVICE_API GraphicsPassLayoutSetup
	{
		static const uint32_t MAX_TARGETS = 8;

		struct Attachment
		{
			ImageFormat format;

			void print(base::IFormatStream& f) const;
			INLINE operator bool() const { return format != ImageFormat::UNKNOWN; }
			INLINE bool operator==(const Attachment& other) const { return format == other.format; }
			INLINE bool operator!=(const Attachment& other) const { return !operator==(other); }
		};

		uint8_t samples = 1;
		Attachment depth;
		Attachment color[MAX_TARGETS];

		//--

		GraphicsPassLayoutSetup();

		//--

		uint64_t key() const;

		void reset();

		void print(base::IFormatStream& f) const;

		bool operator==(const GraphicsPassLayoutSetup& other) const;
		INLINE bool operator!=(const GraphicsPassLayoutSetup& other) const { return !operator==(other); }
	};
#pragma pack(pop)

	static_assert(sizeof(GraphicsPassLayoutSetup) == 10, "Check for gaps");

	//--

} // rendering
