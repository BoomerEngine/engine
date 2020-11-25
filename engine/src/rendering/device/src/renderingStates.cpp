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

    RTTI_BEGIN_TYPE_ENUM(FillMode);
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

	static StaticRenderStatesSetup GDefaultStaticRenderStates;

	const StaticRenderStateDirtyBit StaticRenderStatesSetup::COLOR_MASK_DIRTY_BITS[StaticRenderStatesSetup::MAX_TARGETS] = {
		StaticRenderStateDirtyBit::ColorMask0,
		StaticRenderStateDirtyBit::ColorMask1,
		StaticRenderStateDirtyBit::ColorMask2,
		StaticRenderStateDirtyBit::ColorMask3,
		StaticRenderStateDirtyBit::ColorMask4,
		StaticRenderStateDirtyBit::ColorMask5,
		StaticRenderStateDirtyBit::ColorMask6,
		StaticRenderStateDirtyBit::ColorMask7,
	};

	const StaticRenderStateDirtyBit StaticRenderStatesSetup::BLEND_FUNC_DIRTY_BITS[StaticRenderStatesSetup::MAX_TARGETS] = {
		StaticRenderStateDirtyBit::BlendFunc0,
		StaticRenderStateDirtyBit::BlendFunc1,
		StaticRenderStateDirtyBit::BlendFunc2,
		StaticRenderStateDirtyBit::BlendFunc3,
		StaticRenderStateDirtyBit::BlendFunc4,
		StaticRenderStateDirtyBit::BlendFunc5,
		StaticRenderStateDirtyBit::BlendFunc6,
		StaticRenderStateDirtyBit::BlendFunc7,
	};

	const StaticRenderStateDirtyBit StaticRenderStatesSetup::BLEND_EQUATION_DIRTY_BITS[StaticRenderStatesSetup::MAX_TARGETS] = {
		StaticRenderStateDirtyBit::BlendEquation0,
		StaticRenderStateDirtyBit::BlendEquation1,
		StaticRenderStateDirtyBit::BlendEquation2,
		StaticRenderStateDirtyBit::BlendEquation3,
		StaticRenderStateDirtyBit::BlendEquation4,
		StaticRenderStateDirtyBit::BlendEquation5,
		StaticRenderStateDirtyBit::BlendEquation6,
		StaticRenderStateDirtyBit::BlendEquation7,
	};

	StaticRenderStatesSetup::StaticRenderStatesSetup()
	{
		memzero(this, sizeof(StaticRenderStatesSetup));

		common.fillMode = (int)FillMode::Fill;
		common.depthCompareOp = (int)CompareOp::LessEqual;
		common.stencilFrontFailOp = (int)StencilOp::Keep;
		common.stencilFrontPassOp = (int)StencilOp::Keep;
		common.stencilFrontDepthFailOp = (int)StencilOp::Keep;
		common.stencilFrontCompareOp = (int)CompareOp::Always;
		common.stencilBackFailOp = (int)StencilOp::Keep;
		common.stencilBackPassOp = (int)StencilOp::Keep;
		common.stencilBackDepthFailOp = (int)StencilOp::Keep;
		common.stencilBackCompareOp = (int)CompareOp::Always;
		common.cullMode = (int)CullMode::Disabled;
		common.cullFrontFace = (int)FrontFace::CW;
		common.primitiveTopology = (int)PrimitiveTopology::TriangleList;

		for (uint32_t i = 0; i < MAX_TARGETS; ++i)
		{
			blendStates[i].alphaBlendOp = (int)BlendOp::Add;
			blendStates[i].colorBlendOp = (int)BlendOp::Add;
			blendStates[i].srcColorBlendFactor = (int)BlendFactor::One;
			blendStates[i].srcAlphaBlendFactor = (int)BlendFactor::One;
			blendStates[i].destColorBlendFactor = (int)BlendFactor::Zero;
			blendStates[i].destAlphaBlendFactor = (int)BlendFactor::Zero;
			colorMasks[i] = 0x0F;
		}
	}

	StaticRenderStatesSetup& StaticRenderStatesSetup::DEFAULT_STATES()
	{
		return GDefaultStaticRenderStates;
	}

	bool StaticRenderStatesSetup::operator==(const StaticRenderStatesSetup& other) const
	{
		return false;
	}

	uint64_t StaticRenderStatesSetup::key() const
	{
		StaticRenderStateDirtyFlags copyMask;
		copyMask.enableAll();

		// zero entries that are not used so they don't crapout the CRC
		if (!common.stencilEnabled)
		{
			copyMask -= StaticRenderStateDirtyBit::StencilFrontOps;
			copyMask -= StaticRenderStateDirtyBit::StencilBackOps;
		}
		if (!common.depthEnabled)
		{
			copyMask -= StaticRenderStateDirtyBit::DepthBiasEnabled;
			copyMask -= StaticRenderStateDirtyBit::DepthBoundsEnabled;
			copyMask -= StaticRenderStateDirtyBit::DepthWriteEnabled;
			copyMask -= StaticRenderStateDirtyBit::DepthFunc;
		}
		if (!common.blendingEnabled)
		{
			copyMask -= StaticRenderStateDirtyBit::BlendEquation0;
			copyMask -= StaticRenderStateDirtyBit::BlendEquation1;
			copyMask -= StaticRenderStateDirtyBit::BlendEquation2;
			copyMask -= StaticRenderStateDirtyBit::BlendEquation3;
			copyMask -= StaticRenderStateDirtyBit::BlendEquation4;
			copyMask -= StaticRenderStateDirtyBit::BlendEquation5;
			copyMask -= StaticRenderStateDirtyBit::BlendEquation6;
			copyMask -= StaticRenderStateDirtyBit::BlendEquation7;
			copyMask -= StaticRenderStateDirtyBit::BlendFunc0;
			copyMask -= StaticRenderStateDirtyBit::BlendFunc1;
			copyMask -= StaticRenderStateDirtyBit::BlendFunc2;
			copyMask -= StaticRenderStateDirtyBit::BlendFunc3;
			copyMask -= StaticRenderStateDirtyBit::BlendFunc4;
			copyMask -= StaticRenderStateDirtyBit::BlendFunc5;
			copyMask -= StaticRenderStateDirtyBit::BlendFunc6;
			copyMask -= StaticRenderStateDirtyBit::BlendFunc7;
		}
		if (!common.cullEnabled)
		{
			copyMask -= StaticRenderStateDirtyBit::CullFrontFace;
			copyMask -= StaticRenderStateDirtyBit::CullMode;
		}

		// prepare clear state
		StaticRenderStatesSetup copy;
		copy.apply(*this, &copyMask);

		// now we can make CRC of whole structure
		base::CRC64 crc;
		crc.append(&copy, sizeof(copy));
		return crc;
	}

	void StaticRenderStatesSetup::apply(const StaticRenderStatesSetup& other, const StaticRenderStateDirtyFlags* mask /*= nullptr*/)
	{
		StaticRenderStateDirtyFlags allStats;
		allStats.enableAll();

		const auto applyMask = mask ? allStats : *mask;

		if (applyMask.test(StaticRenderStateDirtyBit::ScissorEnabled))
			common.scissorEnabled = other.common.scissorEnabled;

		if (applyMask.test(StaticRenderStateDirtyBit::StencilEnabled))
			common.stencilEnabled = other.common.stencilEnabled;

		if (applyMask.test(StaticRenderStateDirtyBit::StencilFrontOps))
		{
			common.stencilFrontFailOp = other.common.stencilFrontFailOp;
			common.stencilFrontPassOp = other.common.stencilFrontPassOp;
			common.stencilFrontDepthFailOp = other.common.stencilFrontDepthFailOp;
			common.stencilFrontCompareOp = other.common.stencilFrontCompareOp;
		}

		if (applyMask.test(StaticRenderStateDirtyBit::StencilBackOps))
		{
			common.stencilBackFailOp = other.common.stencilBackFailOp;
			common.stencilBackPassOp = other.common.stencilBackPassOp;
			common.stencilBackDepthFailOp = other.common.stencilBackDepthFailOp;
			common.stencilBackCompareOp = other.common.stencilBackCompareOp;
		}

		if (applyMask.test(StaticRenderStateDirtyBit::DepthBiasEnabled))
			common.depthBiasEnabled = other.common.depthBiasEnabled;

		if (applyMask.test(StaticRenderStateDirtyBit::FillMode))
			common.fillMode = other.common.fillMode;

		if (applyMask.test(StaticRenderStateDirtyBit::PrimitiveRestartEnabled))
			common.primitiveRestartEnabled = other.common.primitiveRestartEnabled;

		if (applyMask.test(StaticRenderStateDirtyBit::PrimitiveTopology))
			common.primitiveTopology = other.common.primitiveTopology;

		if (applyMask.test(StaticRenderStateDirtyBit::CullFrontFace)) 
			common.cullFrontFace = other.common.cullFrontFace;

		if (applyMask.test(StaticRenderStateDirtyBit::CullMode))
			common.cullMode = other.common.cullMode;

		if (applyMask.test(StaticRenderStateDirtyBit::CullEnabled))
			common.cullEnabled = other.common.cullEnabled;

		if (applyMask.test(StaticRenderStateDirtyBit::BlendingEnabled))
			common.blendingEnabled = other.common.blendingEnabled;

		if (applyMask.test(StaticRenderStateDirtyBit::DepthEnabled))
			common.depthEnabled = other.common.depthEnabled;

		if (applyMask.test(StaticRenderStateDirtyBit::DepthWriteEnabled))
			common.depthWriteEnabled = other.common.depthWriteEnabled;

		if (applyMask.test(StaticRenderStateDirtyBit::DepthFunc))
			common.depthCompareOp = other.common.depthCompareOp;

		if (applyMask.test(StaticRenderStateDirtyBit::DepthBoundsEnabled)) 
			common.depthClipEnabled = other.common.depthClipEnabled;
		
		if (applyMask.test(StaticRenderStateDirtyBit::AlphaCoverageEnabled))
			common.alphaToCoverageEnable = other.common.alphaToCoverageEnable;

		if (applyMask.test(StaticRenderStateDirtyBit::AlphaCoverageDitherEnabled))
			common.alphaToCoverageDitherEnable = other.common.alphaToCoverageDitherEnable;

		for (uint8_t i = 0; i < MAX_TARGETS; ++i)
		{
			if (applyMask.test(BLEND_EQUATION_DIRTY_BITS[i]))
			{
				blendStates[i].srcColorBlendFactor = other.blendStates[i].srcColorBlendFactor;
				blendStates[i].destColorBlendFactor = other.blendStates[i].destColorBlendFactor;
				blendStates[i].srcAlphaBlendFactor = other.blendStates[i].srcAlphaBlendFactor;
				blendStates[i].destAlphaBlendFactor = other.blendStates[i].destAlphaBlendFactor;
			}

			if (applyMask.test(BLEND_FUNC_DIRTY_BITS[i]))
			{
				blendStates[i].alphaBlendOp = other.blendStates[i].alphaBlendOp;
				blendStates[i].colorBlendOp = other.blendStates[i].colorBlendOp;
			}

			if (applyMask.test(COLOR_MASK_DIRTY_BITS[i]))
				colorMasks[i] = other.colorMasks[i];
		}
	}

	void StaticRenderStatesSetup::print(base::IFormatStream& f) const
	{
		StaticRenderStateDirtyFlags allFlags;
		allFlags.enableAll();

		print(f, allFlags);
	}

	void StaticRenderStatesSetup::print(base::IFormatStream& f, const StaticRenderStateDirtyFlags& stateMask) const
	{
		if (stateMask.test(StaticRenderStateDirtyBit::ScissorEnabled))
			f.appendf("ScissorEnabled: {}\n", (bool)common.scissorEnabled);

		if (stateMask.test(StaticRenderStateDirtyBit::StencilEnabled))
			f.appendf("StencilEnabled: {}\n", (bool)common.stencilEnabled);

		if (stateMask.test(StaticRenderStateDirtyBit::StencilFrontOps))
		{
			f.appendf("StencilFront: fail={}, depthFail={}, pass={}, op={}\n",
				(StencilOp)common.stencilFrontFailOp,
				(StencilOp)common.stencilFrontDepthFailOp,
				(StencilOp)common.stencilFrontPassOp,
				(CompareOp)common.stencilFrontCompareOp);
		}

		if (stateMask.test(StaticRenderStateDirtyBit::StencilBackOps))
		{
			f.appendf("StencilBack: fail={}, depthFail={}, pass={}, op={}\n",
				(StencilOp)common.stencilBackFailOp,
				(StencilOp)common.stencilBackDepthFailOp,
				(StencilOp)common.stencilBackPassOp,
				(CompareOp)common.stencilBackCompareOp);
		}

		if (stateMask.test(StaticRenderStateDirtyBit::DepthEnabled))
			f.appendf("DepthEnabled: {}\n", (bool)common.depthEnabled);

		if (stateMask.test(StaticRenderStateDirtyBit::DepthWriteEnabled))
			f.appendf("DepthWriteEnabled: {}\n", (bool)common.depthWriteEnabled);

		if (stateMask.test(StaticRenderStateDirtyBit::DepthFunc))
			f.appendf("DepthFunc: {}\n", (CompareOp)common.depthCompareOp);

		if (stateMask.test(StaticRenderStateDirtyBit::DepthBoundsEnabled))
			f.appendf("DepthClipEnabled: {}\n", (bool)common.depthWriteEnabled);

		if (stateMask.test(StaticRenderStateDirtyBit::DepthBiasEnabled))
			f.appendf("DepthBiasEnabled: {}\n", (bool)common.depthBiasEnabled);

		if (stateMask.test(StaticRenderStateDirtyBit::CullEnabled))
			f.appendf("CullEnabled: {}\n", (bool)common.cullEnabled);

		if (stateMask.test(StaticRenderStateDirtyBit::CullFrontFace))
			f.appendf("CullFrontFace: {}\n", (FrontFace)common.cullFrontFace);

		if (stateMask.test(StaticRenderStateDirtyBit::CullMode))
			f.appendf("CullMode: {}\n", (FrontFace)common.cullMode);

		if (stateMask.test(StaticRenderStateDirtyBit::FillMode))
			f.appendf("FillMode: {}\n", (FillMode)common.fillMode);

		if (stateMask.test(StaticRenderStateDirtyBit::PrimitiveRestartEnabled))
			f.appendf("PrimitiveRestartEnabled: {}\n", (bool)common.primitiveRestartEnabled);

		if (stateMask.test(StaticRenderStateDirtyBit::PrimitiveTopology))
			f.appendf("PrititiveTopology: {}\n", (PrimitiveTopology)common.primitiveTopology);

		if (stateMask.test(StaticRenderStateDirtyBit::AlphaCoverageEnabled))
			f.appendf("AlphaCoverageEnabled: {}\n", (bool)common.alphaToCoverageEnable);
			
		if (stateMask.test(StaticRenderStateDirtyBit::AlphaCoverageDitherEnabled))
			f.appendf("AlphaCoverageDitherEnabled: {}\n", (bool)common.alphaToCoverageDitherEnable);

		if (stateMask.test(StaticRenderStateDirtyBit::BlendingEnabled))
			f.appendf("BlendingEnabled: {}\n", (bool)common.blendingEnabled);

		for (uint8_t i = 0; i < MAX_TARGETS; ++i)
		{
			if (stateMask.test(BLEND_EQUATION_DIRTY_BITS[i]))
			{
				f.appendf("BlendEquationColor[{}]: src={} dest={}\n", i, (BlendFactor)blendStates[i].srcColorBlendFactor, (BlendOp)blendStates[i].destColorBlendFactor);
				f.appendf("BlendEquationAlpha[{}]: src={} dest={}\n", i, (BlendFactor)blendStates[i].srcAlphaBlendFactor, (BlendOp)blendStates[i].destAlphaBlendFactor);
			}

			if (stateMask.test(BLEND_FUNC_DIRTY_BITS[i]))
				f.appendf("BlendFunc[{}]: color={} alpha={}\n", i, (BlendOp)blendStates[i].colorBlendOp, (BlendOp)blendStates[i].alphaBlendOp);
			
			if (stateMask.test(COLOR_MASK_DIRTY_BITS[i]))
				f.appendf("ColorMask[{}]: {}\n", i, Hex(colorMasks[i]));
		}
	}

    //--

	StaticRenderStatesBuilder::StaticRenderStatesBuilder()
	{
		reset();
	}

	void StaticRenderStatesBuilder::reset()
	{
		dirtyFlags.clearAll();
		states = StaticRenderStatesSetup();
	}

	void StaticRenderStatesBuilder::colorMask(uint8_t i, uint8_t mask)
	{
		DEBUG_CHECK_RETURN(i < StaticRenderStatesSetup::MAX_TARGETS);
		if (states.colorMasks[i] != mask)
		{
			dirtyFlags |= StaticRenderStatesSetup::COLOR_MASK_DIRTY_BITS[i];
			states.colorMasks[i] = mask;
		}
	}

	void StaticRenderStatesBuilder::blend(bool enabled)
	{
		if (states.common.blendingEnabled != enabled)
		{
			states.common.blendingEnabled = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::BlendingEnabled;
		}
	}

	void StaticRenderStatesBuilder::blendFactor(uint8_t i, BlendFactor src, BlendFactor dest)
	{
		blendFactor(i, src, dest, src, dest);
	}

	void StaticRenderStatesBuilder::blendFactor(uint8_t i, BlendFactor srcColor, BlendFactor destColor, BlendFactor srcAlpha, BlendFactor destAlpha)
	{
		DEBUG_CHECK_RETURN(i < StaticRenderStatesSetup::MAX_TARGETS);

		auto& state = states.blendStates[i];
		if ((BlendFactor)state.srcColorBlendFactor != srcColor || (BlendFactor)state.destColorBlendFactor != destColor
			|| (BlendFactor)state.srcAlphaBlendFactor != srcAlpha || (BlendFactor)state.destAlphaBlendFactor != destAlpha)
		{
			state.srcColorBlendFactor = (int)srcColor;
			state.destColorBlendFactor = (int)destColor;
			state.srcAlphaBlendFactor = (int)srcAlpha;
			state.destAlphaBlendFactor = (int)destAlpha;
			dirtyFlags |= StaticRenderStatesSetup::BLEND_EQUATION_DIRTY_BITS[i];
		}
	}

	void StaticRenderStatesBuilder::blendOp(uint8_t i, BlendOp op)
	{
		blendOp(i, op, op);
	}

	void StaticRenderStatesBuilder::blendOp(uint8_t i, BlendOp color, BlendOp alpha)
	{
		DEBUG_CHECK_RETURN(i < StaticRenderStatesSetup::MAX_TARGETS);

		auto& state = states.blendStates[i];
		if ((BlendOp)state.colorBlendOp != color|| (BlendOp)state.alphaBlendOp != alpha)
		{
			state.colorBlendOp = (int)color;
			state.alphaBlendOp = (int)alpha;
			dirtyFlags |= StaticRenderStatesSetup::BLEND_FUNC_DIRTY_BITS[i];
		}
	}

	void StaticRenderStatesBuilder::cull(bool enabled)
	{
		if (states.common.cullEnabled != enabled)
		{
			states.common.cullEnabled = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::CullEnabled;
		}
	}

	void StaticRenderStatesBuilder::cullMode(CullMode mode)
	{
		if ((CullMode)states.common.cullMode != mode)
		{
			states.common.cullMode = (int)mode;
			dirtyFlags |= StaticRenderStateDirtyBit::CullMode;
		}
	}

	void StaticRenderStatesBuilder::cullFrontFace(FrontFace mode)
	{
		if ((FrontFace)states.common.cullFrontFace != mode)
		{
			states.common.cullFrontFace = (int)mode;
			dirtyFlags |= StaticRenderStateDirtyBit::CullFrontFace;
		}
	}

	void StaticRenderStatesBuilder::fill(FillMode mode)
	{
		if ((FillMode)states.common.fillMode != mode)
		{
			states.common.fillMode = (int)mode;
			dirtyFlags |= StaticRenderStateDirtyBit::FillMode;
		}
	}

	void StaticRenderStatesBuilder::scissorState(bool enabled)
	{
		if (states.common.scissorEnabled != enabled)
		{
			states.common.scissorEnabled = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::ScissorEnabled;
		}
	}

	void StaticRenderStatesBuilder::stencil(bool enabled)
	{
		if (states.common.stencilEnabled != enabled)
		{
			states.common.stencilEnabled = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::StencilEnabled;
		}
	}

	void StaticRenderStatesBuilder::stencilAll(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp)
	{
		stencilFront(compareOp, failOp, depthFailOp, passOp);
		stencilBack(compareOp, failOp, depthFailOp, passOp);
	}

	void StaticRenderStatesBuilder::stencilFront(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp)
	{
		if ((CompareOp)states.common.stencilFrontCompareOp != compareOp
			|| (StencilOp)states.common.stencilFrontFailOp != failOp
			|| (StencilOp)states.common.stencilFrontPassOp != passOp
			|| (StencilOp)states.common.stencilFrontDepthFailOp != depthFailOp)
		{
			states.common.stencilFrontCompareOp = (int)compareOp;
			states.common.stencilFrontFailOp = (int)failOp;
			states.common.stencilFrontPassOp = (int)passOp;
			states.common.stencilFrontDepthFailOp = (int)depthFailOp;
			dirtyFlags |= StaticRenderStateDirtyBit::StencilFrontOps;
		}
	}

	void StaticRenderStatesBuilder::stencilBack(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp)
	{
		if ((CompareOp)states.common.stencilBackCompareOp != compareOp
			|| (StencilOp)states.common.stencilBackFailOp != failOp
			|| (StencilOp)states.common.stencilBackPassOp != passOp
			|| (StencilOp)states.common.stencilBackDepthFailOp != depthFailOp)
		{
			states.common.stencilBackCompareOp = (int)compareOp;
			states.common.stencilBackFailOp = (int)failOp;
			states.common.stencilBackPassOp = (int)passOp;
			states.common.stencilBackDepthFailOp = (int)depthFailOp;
			dirtyFlags |= StaticRenderStateDirtyBit::StencilBackOps;
		}
	}

	void StaticRenderStatesBuilder::depth(bool enable)
	{
		if (states.common.depthEnabled != enable)
		{
			states.common.depthEnabled = enable;
			dirtyFlags |= StaticRenderStateDirtyBit::DepthEnabled;
		}
	}

	void StaticRenderStatesBuilder::depthWrite(bool enable)
	{
		if (states.common.depthWriteEnabled != enable)
		{
			states.common.depthWriteEnabled = enable;
			dirtyFlags |= StaticRenderStateDirtyBit::DepthWriteEnabled;
		}
	}

	void StaticRenderStatesBuilder::depthFunc(CompareOp func)
	{
		if ((CompareOp)states.common.depthCompareOp != func)
		{
			states.common.depthCompareOp = (int)func;
			dirtyFlags |= StaticRenderStateDirtyBit::DepthFunc;
		}
	}

	void StaticRenderStatesBuilder::depthClip(bool enabled)
	{
		if (states.common.depthClipEnabled != enabled)
		{
			states.common.depthClipEnabled = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::DepthBoundsEnabled;
		}
	}

	void StaticRenderStatesBuilder::depthBias(bool enabled)
	{
		if (states.common.depthBiasEnabled != enabled)
		{
			states.common.depthBiasEnabled = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::DepthBiasEnabled;
		}
	}

	void StaticRenderStatesBuilder::primitiveTopology(PrimitiveTopology topology)
	{
		if ((PrimitiveTopology)states.common.primitiveTopology != topology)
		{
			states.common.primitiveTopology = (int)topology;
			dirtyFlags |= StaticRenderStateDirtyBit::PrimitiveTopology;
		}
	}

	void StaticRenderStatesBuilder::primitiveRestart(bool enabled)
	{
		if (states.common.primitiveRestartEnabled != enabled)
		{
			states.common.primitiveRestartEnabled = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::PrimitiveRestartEnabled;
		}
	}

	//void StaticRenderStatesBuilder::multisample(uint8_t numSamples);

	void StaticRenderStatesBuilder::alphaToCoverage(bool enabled)
	{
		if (states.common.alphaToCoverageEnable != enabled)
		{
			states.common.alphaToCoverageEnable = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::AlphaCoverageEnabled;
		}
	}

	void StaticRenderStatesBuilder::alphaToCoverageDither(bool enabled)
	{
		if (states.common.alphaToCoverageDitherEnable != enabled)
		{
			states.common.alphaToCoverageDitherEnable = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::AlphaCoverageDitherEnabled;
		}
	}

	void StaticRenderStatesBuilder::alphaToOne(bool enabled)
	{
		/*if (states.common.alphaToCoverageDitherEnable != enabled)
		{
			states.common.alphaToCoverageDitherEnable = enabled;
			dirtyFlags |= StaticRenderStateDirtyBit::AlphaCoverageDitherEnabled;
		}*/
	}

	//--

	GraphicsPassLayoutSetup::GraphicsPassLayoutSetup()
	{
		reset();
	}

	void GraphicsPassLayoutSetup::reset()
	{
		samples = 1;
		depth = Attachment();
		for (uint32_t i=0; i<MAX_TARGETS; ++i)
			color[i] = Attachment();
	}
	
	uint64_t GraphicsPassLayoutSetup::key() const
	{
		base::CRC64 crc;
		crc.append(this, sizeof(GraphicsPassLayoutSetup));
		return crc;
	}

	void GraphicsPassLayoutSetup::Attachment::print(base::IFormatStream& f) const
	{
		f << format;
	}

	void GraphicsPassLayoutSetup::print(base::IFormatStream& f) const
	{
		if (depth)
		{
			f.appendf("DEPTH: {}", depth);
			if (samples)
				f.appendf(" MSAA x{}", samples);
			f << "\n";
		}

		for (uint32_t i=0; i<MAX_TARGETS; ++i)
		{
			if (!color[i])
				break;

			f.appendf("COLOR[{}]: {}", i, color[i]);
			if (samples)
				f.appendf(" MSAA x{}", samples);
			f << "\n";
		}
	}

	bool GraphicsPassLayoutSetup::operator==(const GraphicsPassLayoutSetup& other) const
	{
		if (depth != other.depth || samples != other.samples)
			return false;

		for (uint32_t i = 0; i < MAX_TARGETS; ++i)
		{
			if (color[i] != other.color[i])
				return false;
			if (!color[i])
				break;
		}

		return true;
	}

	//--

} // rendering