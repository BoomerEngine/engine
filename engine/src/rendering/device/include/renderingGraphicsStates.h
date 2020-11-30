/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: data #]
***/

#pragma once

#include "renderingImageFormat.h"
#include "base/containers/include/bitSet.h"

namespace rendering
{
	//---

	// NOTE: some enums are expected to fit within specicied bit count

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

	// bit describing which render state is dirty
	enum class GraphicRenderStatesBit : uint64_t
	{
		FillMode,

		CullEnabled,
		CullMode,
		CullFrontFace,

		DepthEnabled,
		DepthWriteEnabled,
		DepthFunc,

		DepthBiasEnabled,
		DepthBiasValue,
		DepthBiasSlope,
		DepthBiasClamp,

		ScissorEnabled,

		StencilEnabled,
		StencilFrontFunc,
		StencilFrontFailOp,
		StencilFrontPassOp,
		StencilFrontDepthFailOp,
		StencilBackFunc,
		StencilBackFailOp,
		StencilBackPassOp,
		StencilBackDepthFailOp,
		StencilReadMask,
		StencilWriteMask,

		PrimitiveRestartEnabled,
		PrimitiveTopology,

		BlendingEnabled,

		AlphaCoverageEnabled,
		AlphaCoverageDitherEnabled,

		ColorMask0,
		ColorMask1,
		ColorMask2,
		ColorMask3,
		ColorMask4,
		ColorMask5,
		ColorMask6,
		ColorMask7,

		BlendColorOp0,
		BlendColorSrc0,
		BlendColorDest0,
		BlendAlphaOp0,
		BlendAlphaSrc0,
		BlendAlphaDest0,

		BlendColorOp1,
		BlendColorSrc1,
		BlendColorDest1,
		BlendAlphaOp1,
		BlendAlphaSrc1,
		BlendAlphaDest1,

		BlendColorOp2,
		BlendColorSrc2,
		BlendColorDest2,
		BlendAlphaOp2,
		BlendAlphaSrc2,
		BlendAlphaDest2,

		BlendColorOp3,
		BlendColorSrc3,
		BlendColorDest3,
		BlendAlphaOp3,
		BlendAlphaSrc3,
		BlendAlphaDest3,

		BlendColorOp4,
		BlendColorSrc4,
		BlendColorDest4,
		BlendAlphaOp4,
		BlendAlphaSrc4,
		BlendAlphaDest4,

		BlendColorOp5,
		BlendColorSrc5,
		BlendColorDest5,
		BlendAlphaOp5,
		BlendAlphaSrc5,
		BlendAlphaDest5,

		BlendColorOp6,
		BlendColorSrc6,
		BlendColorDest6,
		BlendAlphaOp6,
		BlendAlphaSrc6,
		BlendAlphaDest6,

		BlendColorOp7,
		BlendColorSrc7,
		BlendColorDest7,
		BlendAlphaOp7,
		BlendAlphaSrc7,
		BlendAlphaDest7,

		MAX,
	};

	static_assert((int)GraphicRenderStatesBit::MAX <= 128, "To many render states to track");

	///--

	// TOD: migrate to base as "bit flags" or something
	struct RENDERING_DEVICE_API GraphicRenderStatesMask
	{
		//--

		INLINE GraphicRenderStatesMask()
		{
			for (int i = 0; i < NUM_WORDS; ++i)
				words[i] = 0;
		}

		INLINE GraphicRenderStatesMask(const GraphicRenderStatesMask& other)
		{
			for (int i = 0; i < NUM_WORDS; ++i)
				words[i] = other.words[i];
		}

		INLINE GraphicRenderStatesMask& operator=(const GraphicRenderStatesMask& other)
		{
			for (int i = 0; i < NUM_WORDS; ++i)
				words[i] = other.words[i];
			return *this;
		}

		INLINE GraphicRenderStatesMask& operator-=(GraphicRenderStatesBit bit)
		{
			clearBit((int)bit);
			return *this;
		}

		INLINE GraphicRenderStatesMask& operator|=(GraphicRenderStatesBit bit)
		{
			setBit((int)bit);
			return *this;
		}

		INLINE GraphicRenderStatesMask& operator|=(const GraphicRenderStatesMask& other)
		{
			for (int i = 0; i < NUM_WORDS; ++i)
				words[i] |= other.words[i];
			return *this;
		}

		INLINE bool test(GraphicRenderStatesBit bit) const
		{
			return testBit((int)bit);
		}

		//--

		static const auto WORD_SIZE = 32;
		static const auto NUM_WORDS = ((int)GraphicRenderStatesBit::MAX + WORD_SIZE) / WORD_SIZE;
		uint32_t words[NUM_WORDS];

		//--

	private:
		ALWAYS_INLINE void clearBit(int bit)
		{
			words[bit / WORD_SIZE] &= ~(1U << (bit % WORD_SIZE));
		}

		ALWAYS_INLINE void setBit(int bit)
		{
			words[bit / WORD_SIZE] |= (1U << (bit % WORD_SIZE));
		}

		ALWAYS_INLINE bool testBit(int bit) const
		{
			return 0 != (words[bit / WORD_SIZE] & (1U << (bit % WORD_SIZE)));
		}
	};

	//--

	/// helper structure for collecting (setting) all STATIC render states that are usually baked in
	struct RENDERING_DEVICE_API GraphicsRenderStatesSetup
	{
		static const uint8_t MAX_TARGETS = 8;

		static const GraphicRenderStatesBit COLOR_MASK_DIRTY_BITS[MAX_TARGETS];
		static const GraphicRenderStatesBit BLEND_COLOR_OP_DIRTY_BITS[MAX_TARGETS];
		static const GraphicRenderStatesBit BLEND_ALPHA_OP_DIRTY_BITS[MAX_TARGETS];
		static const GraphicRenderStatesBit BLEND_COLOR_SRC_FACTOR_DIRTY_BITS[MAX_TARGETS];
		static const GraphicRenderStatesBit BLEND_COLOR_DEST_FACTOR_DIRTY_BITS[MAX_TARGETS];
		static const GraphicRenderStatesBit BLEND_ALPHA_SRC_FACTOR_DIRTY_BITS[MAX_TARGETS];
		static const GraphicRenderStatesBit BLEND_ALPHA_DEST_FACTOR_DIRTY_BITS[MAX_TARGETS];

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

		struct DepthStencilExtra
		{
			int depthBias = 0;
			float depthBiasSlope = 0.0f;
			float depthBiasClamp = 0.0f;

			uint8_t stencilWriteMask = 0xFF;
			uint8_t stencilReadMask = 0xFF;
		};

		//--

		GraphicRenderStatesMask mask;

		CommonState common;
		DepthStencilExtra depthStencilStates;
		BlendState blendStates[MAX_TARGETS];
		uint8_t colorMasks[MAX_TARGETS];

		//--

		static GraphicsRenderStatesSetup& DEFAULT_STATES();

		//--

		GraphicsRenderStatesSetup();

		// calculate unique key of the render states
		// NOTE: expensive
		// NOTE: only masked states are used
		uint64_t key() const;

		/// reset to default states (mask and values are cleared)
		void reset();

		// apply states (copy their values here, only masked states are copied)
		void apply(const GraphicsRenderStatesSetup& other, const GraphicRenderStatesMask* mask = nullptr);

		// debug print
		void print(base::IFormatStream& f) const;

		// debug print of only masked states
		void printMasked(base::IFormatStream& f, const GraphicRenderStatesMask& stateMask) const;

		//--

		// configure from key+value (mostly used in shader compilation)
		bool configure(base::StringID key, base::StringView value, base::IFormatStream& err);

		//--

		/// change color mask for given RT
		void colorMask(uint8_t rtIndex, uint8_t mask);

		//--

		/// enable/disable alpha blending
		void blend(bool enabled);

		/// set blend equation for a given render target
		void blendFactor(uint8_t renderTargetIndex, BlendFactor src, BlendFactor dest);

		/// set blend equation for a given render target - separate color and alpha
		void blendFactor(uint8_t renderTargetIndex, BlendFactor srcColor, BlendFactor destColor, BlendFactor srcAlpha, BlendFactor destAlpha);

		/// set blend operation
		void blendOp(uint8_t renderTargetIndex, BlendOp op);

		/// set blend operation - separated color and alpha one
		void blendOp(uint8_t renderTargetIndex, BlendOp color, BlendOp alpha);

		//--

		/// enable/disable face culling
		void cull(bool enabled);

		// change the cull mode
		void cullMode(CullMode mode);

		// change which face winding is considered "front"
		void cullFrontFace(FrontFace mode);

		//--

		/// set polygon mode
		void fill(FillMode mode);

		//--

		/// enable/disable scissoring
		void scissorState(bool enabled);

		//--

		/// enable/disable stencil operations
		void stencil(bool enabled);

		// configure stencil for both back and front
		void stencilAll(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp);

		// configure stencil for front faces
		void stencilFront(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp);

		// configure stencil for back faces
		void stencilBack(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp);
		
		// change stencil read (compare) mask
		void stencilReadMask(uint8_t mask);

		// change stencil write mask
		void stencilWriteMask(uint8_t mask);

		//--

		// enable/disable depth operations
		void depth(bool enable);

		// enable/disable depth writes
		void depthWrite(bool enable);

		// change depth function
		void depthFunc(CompareOp func);

		//--

		// enable/disable depth bias
		void depthBias(bool enabled);

		// set depth bias value
		void depthBiasValue(int value);

		// set depth bias slope
		void depthBiasSlope(float value);

		// set depth bias clamp
		void depthBiasClamp(float value);

		//---

		// change topology of input primitives
		void primitiveTopology(PrimitiveTopology topology);

		// enable/disable primitive restart
		void primitiveRestart(bool enabled);

		//--

		/// enable alpha to coverage
		void alphaToCoverage(bool enabled);

		/// enable alpha to coverage dither
		void alphaToCoverageDither(bool enabled);

		/// enable alpha to one feature
		void alphaToOne(bool enabled);

		//--

		// compare if states are actually the same
		// NOTE: only masked states are compared
		bool operator==(const GraphicsRenderStatesSetup& other) const;
		INLINE bool operator!=(const GraphicsRenderStatesSetup& other) const { return !operator==(other); }

		//--

		// Custom type implementation requirements
		void writeBinary(base::stream::OpcodeWriter& stream) const;
		void readBinary(base::stream::OpcodeReader& stream);
	};

} // rendering
