/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: data #]
***/

#include "build.h"
#include "graphicsStates.h"
#include "core/object/include/streamOpcodeWriter.h"
#include "core/object/include/streamOpcodeReader.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//---

RTTI_BEGIN_TYPE_ENUM(ShaderStage);
	RTTI_ENUM_OPTION(Pixel);
	RTTI_ENUM_OPTION(Vertex);
	RTTI_ENUM_OPTION(Geometry);
	RTTI_ENUM_OPTION(Hull);
	RTTI_ENUM_OPTION(Domain);
	RTTI_ENUM_OPTION(Compute);
	RTTI_ENUM_OPTION(Mesh);
	RTTI_ENUM_OPTION(Task);
RTTI_END_TYPE()

//---
	
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

RTTI_BEGIN_CUSTOM_TYPE(GraphicsRenderStatesSetup);
	RTTI_TYPE_TRAIT().noDestructor();
	RTTI_BIND_NATIVE_BINARY_SERIALIZATION(GraphicsRenderStatesSetup);
	RTTI_BIND_NATIVE_PRINT(GraphicsRenderStatesSetup);
	RTTI_BIND_NATIVE_COPY(GraphicsRenderStatesSetup);
	RTTI_BIND_NATIVE_COMPARE(GraphicsRenderStatesSetup);
RTTI_END_TYPE();

//--

static GraphicsRenderStatesSetup GDefaultStaticRenderStates;

#define GENERATE_BIT_TABLE(name, stem)  \
const GraphicRenderStatesBit GraphicsRenderStatesSetup::##name[GraphicsRenderStatesSetup::MAX_TARGETS] = { \
	GraphicRenderStatesBit::##stem##0, \
	GraphicRenderStatesBit::##stem##1, \
	GraphicRenderStatesBit::##stem##2, \
	GraphicRenderStatesBit::##stem##3, \
	GraphicRenderStatesBit::##stem##4, \
	GraphicRenderStatesBit::##stem##5, \
	GraphicRenderStatesBit::##stem##6, \
	GraphicRenderStatesBit::##stem##7 };

GENERATE_BIT_TABLE(COLOR_MASK_DIRTY_BITS, ColorMask);
GENERATE_BIT_TABLE(BLEND_COLOR_OP_DIRTY_BITS, BlendColorOp);
GENERATE_BIT_TABLE(BLEND_ALPHA_OP_DIRTY_BITS, BlendAlphaOp);
GENERATE_BIT_TABLE(BLEND_COLOR_SRC_FACTOR_DIRTY_BITS, BlendColorSrc);
GENERATE_BIT_TABLE(BLEND_COLOR_DEST_FACTOR_DIRTY_BITS, BlendColorDest);
GENERATE_BIT_TABLE(BLEND_ALPHA_SRC_FACTOR_DIRTY_BITS, BlendAlphaSrc);
GENERATE_BIT_TABLE(BLEND_ALPHA_DEST_FACTOR_DIRTY_BITS, BlendAlphaDest);

GraphicsRenderStatesSetup::GraphicsRenderStatesSetup()
{
	reset();
}

GraphicsRenderStatesSetup& GraphicsRenderStatesSetup::DEFAULT_STATES()
{
	return GDefaultStaticRenderStates;
}

bool GraphicsRenderStatesSetup::operator==(const GraphicsRenderStatesSetup& other) const
{
	return key() == other.key();
}

uint64_t GraphicsRenderStatesSetup::key() const
{
	auto copyMask = mask;

	// zero entries that are not used so they don't crap out the CRC
	if (!common.stencilEnabled || !mask.test(GraphicRenderStatesBit::StencilEnabled))
	{
		copyMask -= GraphicRenderStatesBit::StencilFrontFunc;
		copyMask -= GraphicRenderStatesBit::StencilFrontFailOp;
		copyMask -= GraphicRenderStatesBit::StencilFrontPassOp;
		copyMask -= GraphicRenderStatesBit::StencilFrontDepthFailOp;
		copyMask -= GraphicRenderStatesBit::StencilBackFunc;
		copyMask -= GraphicRenderStatesBit::StencilBackFailOp;
		copyMask -= GraphicRenderStatesBit::StencilBackPassOp;
		copyMask -= GraphicRenderStatesBit::StencilBackDepthFailOp;
		copyMask -= GraphicRenderStatesBit::StencilReadMask;
		copyMask -= GraphicRenderStatesBit::StencilWriteMask;
	}
	if (!common.depthEnabled ||!mask.test(GraphicRenderStatesBit::DepthEnabled))
	{
		copyMask -= GraphicRenderStatesBit::DepthBiasEnabled;
		copyMask -= GraphicRenderStatesBit::DepthWriteEnabled;
		copyMask -= GraphicRenderStatesBit::DepthFunc;

		copyMask -= GraphicRenderStatesBit::DepthBiasClamp;
		copyMask -= GraphicRenderStatesBit::DepthBiasSlope;
		copyMask -= GraphicRenderStatesBit::DepthBiasValue;
	}
	else if (!common.depthBiasEnabled || !mask.test(GraphicRenderStatesBit::DepthBiasEnabled))
	{
		copyMask -= GraphicRenderStatesBit::DepthBiasClamp;
		copyMask -= GraphicRenderStatesBit::DepthBiasSlope;
		copyMask -= GraphicRenderStatesBit::DepthBiasValue;
	}

	if (!common.blendingEnabled || !mask.test(GraphicRenderStatesBit::BlendingEnabled))
	{
		for (int i = 0; i < MAX_TARGETS; ++i)
		{
			copyMask -= BLEND_COLOR_OP_DIRTY_BITS[i];
			copyMask -= BLEND_COLOR_SRC_FACTOR_DIRTY_BITS[i];
			copyMask -= BLEND_COLOR_DEST_FACTOR_DIRTY_BITS[i];
			copyMask -= BLEND_ALPHA_OP_DIRTY_BITS[i];
			copyMask -= BLEND_ALPHA_SRC_FACTOR_DIRTY_BITS[i];
			copyMask -= BLEND_ALPHA_DEST_FACTOR_DIRTY_BITS[i];				
		}
	}

	if (!common.cullEnabled || !mask.test(GraphicRenderStatesBit::CullEnabled))
	{
		copyMask -= GraphicRenderStatesBit::CullFrontFace;
		copyMask -= GraphicRenderStatesBit::CullMode;
	}

	// prepare clear state
	GraphicsRenderStatesSetup copy;
	memzero(&copy, sizeof(copy));
	copy.apply(*this, &copyMask);

	// now we can make CRC of whole structure
	CRC64 crc;
	crc.append(&copy, sizeof(copy));
	return crc;
}

void GraphicsRenderStatesSetup::apply(const GraphicsRenderStatesSetup& other, const GraphicRenderStatesMask* customApplyMask /*= nullptr*/)
{
	auto applyMask = customApplyMask ? *customApplyMask : other.mask;
	mask |= applyMask;

	if (applyMask.test(GraphicRenderStatesBit::ScissorEnabled))
		common.scissorEnabled = other.common.scissorEnabled;

	if (applyMask.test(GraphicRenderStatesBit::StencilEnabled))
		common.stencilEnabled = other.common.stencilEnabled;

	if (applyMask.test(GraphicRenderStatesBit::StencilReadMask))
		depthStencilStates.stencilReadMask = other.depthStencilStates.stencilReadMask;

	if (applyMask.test(GraphicRenderStatesBit::StencilWriteMask))
		depthStencilStates.stencilWriteMask = other.depthStencilStates.stencilWriteMask;

	if (applyMask.test(GraphicRenderStatesBit::StencilFrontFunc))
		common.stencilFrontCompareOp = other.common.stencilFrontCompareOp;

	if (applyMask.test(GraphicRenderStatesBit::StencilFrontFailOp))
		common.stencilFrontFailOp = other.common.stencilFrontFailOp;

	if (applyMask.test(GraphicRenderStatesBit::StencilFrontPassOp))
		common.stencilFrontPassOp = other.common.stencilFrontPassOp;

	if (applyMask.test(GraphicRenderStatesBit::StencilFrontDepthFailOp))
		common.stencilFrontDepthFailOp = other.common.stencilFrontDepthFailOp;

	if (applyMask.test(GraphicRenderStatesBit::StencilBackFunc))
		common.stencilBackCompareOp = other.common.stencilBackCompareOp;

	if (applyMask.test(GraphicRenderStatesBit::StencilBackFailOp))
		common.stencilBackFailOp = other.common.stencilBackFailOp;

	if (applyMask.test(GraphicRenderStatesBit::StencilBackPassOp))
		common.stencilBackPassOp = other.common.stencilBackPassOp;

	if (applyMask.test(GraphicRenderStatesBit::StencilBackDepthFailOp))
		common.stencilBackDepthFailOp = other.common.stencilBackDepthFailOp;

	if (applyMask.test(GraphicRenderStatesBit::DepthBiasEnabled))
		common.depthBiasEnabled = other.common.depthBiasEnabled;

	if (applyMask.test(GraphicRenderStatesBit::DepthBiasValue))
		depthStencilStates.depthBias = other.depthStencilStates.depthBias;

	if (applyMask.test(GraphicRenderStatesBit::DepthBiasSlope))
		depthStencilStates.depthBiasSlope = other.depthStencilStates.depthBiasSlope;

	if (applyMask.test(GraphicRenderStatesBit::DepthBiasClamp))
		depthStencilStates.depthBiasClamp = other.depthStencilStates.depthBiasClamp;

	if (applyMask.test(GraphicRenderStatesBit::FillMode))
		common.fillMode = other.common.fillMode;

	if (applyMask.test(GraphicRenderStatesBit::PrimitiveRestartEnabled))
		common.primitiveRestartEnabled = other.common.primitiveRestartEnabled;

	if (applyMask.test(GraphicRenderStatesBit::PrimitiveTopology))
		common.primitiveTopology = other.common.primitiveTopology;

	if (applyMask.test(GraphicRenderStatesBit::CullFrontFace)) 
		common.cullFrontFace = other.common.cullFrontFace;

	if (applyMask.test(GraphicRenderStatesBit::CullMode))
		common.cullMode = other.common.cullMode;

	if (applyMask.test(GraphicRenderStatesBit::CullEnabled))
		common.cullEnabled = other.common.cullEnabled;

	if (applyMask.test(GraphicRenderStatesBit::BlendingEnabled))
		common.blendingEnabled = other.common.blendingEnabled;

	if (applyMask.test(GraphicRenderStatesBit::DepthEnabled))
		common.depthEnabled = other.common.depthEnabled;

	if (applyMask.test(GraphicRenderStatesBit::DepthWriteEnabled))
		common.depthWriteEnabled = other.common.depthWriteEnabled;

	if (applyMask.test(GraphicRenderStatesBit::DepthFunc))
		common.depthCompareOp = other.common.depthCompareOp;

	if (applyMask.test(GraphicRenderStatesBit::AlphaCoverageEnabled))
		common.alphaToCoverageEnable = other.common.alphaToCoverageEnable;

	if (applyMask.test(GraphicRenderStatesBit::AlphaCoverageDitherEnabled))
		common.alphaToCoverageDitherEnable = other.common.alphaToCoverageDitherEnable;

	for (uint8_t i = 0; i < MAX_TARGETS; ++i)
	{
		if (applyMask.test(BLEND_COLOR_OP_DIRTY_BITS[i]))
			blendStates[i].colorBlendOp = other.blendStates[i].colorBlendOp;

		if (applyMask.test(BLEND_COLOR_SRC_FACTOR_DIRTY_BITS[i]))
			blendStates[i].srcColorBlendFactor = other.blendStates[i].srcColorBlendFactor;

		if (applyMask.test(BLEND_COLOR_DEST_FACTOR_DIRTY_BITS[i]))
			blendStates[i].destColorBlendFactor = other.blendStates[i].destColorBlendFactor;

		if (applyMask.test(BLEND_ALPHA_OP_DIRTY_BITS[i]))
			blendStates[i].alphaBlendOp = other.blendStates[i].alphaBlendOp;

		if (applyMask.test(BLEND_ALPHA_SRC_FACTOR_DIRTY_BITS[i]))
			blendStates[i].srcAlphaBlendFactor = other.blendStates[i].srcAlphaBlendFactor;

		if (applyMask.test(BLEND_ALPHA_DEST_FACTOR_DIRTY_BITS[i]))
			blendStates[i].destAlphaBlendFactor = other.blendStates[i].destAlphaBlendFactor;

		if (applyMask.test(COLOR_MASK_DIRTY_BITS[i]))
			colorMasks[i] = other.colorMasks[i];
	}
}

void GraphicsRenderStatesSetup::print(IFormatStream& f) const
{
	printMasked(f, mask);
}

void GraphicsRenderStatesSetup::printMasked(IFormatStream& f, const GraphicRenderStatesMask& stateMask) const
{
	if (stateMask.test(GraphicRenderStatesBit::ScissorEnabled))
		f.appendf("ScissorEnabled: {}\n", (bool)common.scissorEnabled);

	if (stateMask.test(GraphicRenderStatesBit::StencilEnabled))
		f.appendf("StencilEnabled: {}\n", (bool)common.stencilEnabled);

	if (stateMask.test(GraphicRenderStatesBit::StencilFrontFunc))
		f.appendf("StencilFrontFunc: {}\n", (CompareOp)common.stencilFrontCompareOp);

	if (stateMask.test(GraphicRenderStatesBit::StencilFrontPassOp))
		f.appendf("StencilFrontPassOp: {}\n", (StencilOp)common.stencilFrontPassOp);

	if (stateMask.test(GraphicRenderStatesBit::StencilFrontFailOp))
		f.appendf("StencilFrontFailOp: {}\n", (StencilOp)common.stencilFrontFailOp);

	if (stateMask.test(GraphicRenderStatesBit::StencilFrontDepthFailOp))
		f.appendf("StencilFrontDepthFailOp: {}\n", (StencilOp)common.stencilFrontDepthFailOp);
		
	if (stateMask.test(GraphicRenderStatesBit::StencilBackFunc))
		f.appendf("StencilBackFunc: {}\n", (CompareOp)common.stencilBackCompareOp);

	if (stateMask.test(GraphicRenderStatesBit::StencilBackPassOp))
		f.appendf("StencilBackPassOp: {}\n", (StencilOp)common.stencilBackPassOp);

	if (stateMask.test(GraphicRenderStatesBit::StencilBackFailOp))
		f.appendf("StencilBackFailOp: {}\n", (StencilOp)common.stencilBackFailOp);

	if (stateMask.test(GraphicRenderStatesBit::StencilBackDepthFailOp))
		f.appendf("StencilBackDepthFailOp: {}\n", (StencilOp)common.stencilBackDepthFailOp);

	if (stateMask.test(GraphicRenderStatesBit::DepthEnabled))
		f.appendf("DepthEnabled: {}\n", (bool)common.depthEnabled);

	if (stateMask.test(GraphicRenderStatesBit::DepthWriteEnabled))
		f.appendf("DepthWriteEnabled: {}\n", (bool)common.depthWriteEnabled);

	if (stateMask.test(GraphicRenderStatesBit::DepthFunc))
		f.appendf("DepthFunc: {}\n", (CompareOp)common.depthCompareOp);

	if (stateMask.test(GraphicRenderStatesBit::DepthBiasEnabled))
		f.appendf("DepthBiasEnabled: {}\n", (bool)common.depthBiasEnabled);

	if (stateMask.test(GraphicRenderStatesBit::DepthBiasValue))
		f.appendf("DepthBiasValue: {}\n", depthStencilStates.depthBias);

	if (stateMask.test(GraphicRenderStatesBit::DepthBiasSlope))
		f.appendf("DepthBiasSlop: {}\n", depthStencilStates.depthBiasSlope);

	if (stateMask.test(GraphicRenderStatesBit::DepthBiasClamp))
		f.appendf("DepthBiasClamp: {}\n", depthStencilStates.depthBiasClamp);

	if (stateMask.test(GraphicRenderStatesBit::CullEnabled))
		f.appendf("CullEnabled: {}\n", (bool)common.cullEnabled);

	if (stateMask.test(GraphicRenderStatesBit::CullFrontFace))
		f.appendf("CullFrontFace: {}\n", (FrontFace)common.cullFrontFace);

	if (stateMask.test(GraphicRenderStatesBit::CullMode))
		f.appendf("CullMode: {}\n", (FrontFace)common.cullMode);

	if (stateMask.test(GraphicRenderStatesBit::FillMode))
		f.appendf("FillMode: {}\n", (FillMode)common.fillMode);

	if (stateMask.test(GraphicRenderStatesBit::PrimitiveRestartEnabled))
		f.appendf("PrimitiveRestartEnabled: {}\n", (bool)common.primitiveRestartEnabled);

	if (stateMask.test(GraphicRenderStatesBit::PrimitiveTopology))
		f.appendf("PrititiveTopology: {}\n", (PrimitiveTopology)common.primitiveTopology);

	if (stateMask.test(GraphicRenderStatesBit::AlphaCoverageEnabled))
		f.appendf("AlphaCoverageEnabled: {}\n", (bool)common.alphaToCoverageEnable);
			
	if (stateMask.test(GraphicRenderStatesBit::AlphaCoverageDitherEnabled))
		f.appendf("AlphaCoverageDitherEnabled: {}\n", (bool)common.alphaToCoverageDitherEnable);

	if (stateMask.test(GraphicRenderStatesBit::BlendingEnabled))
		f.appendf("BlendingEnabled: {}\n", (bool)common.blendingEnabled);

	for (uint8_t i = 0; i < MAX_TARGETS; ++i)
	{
		if (stateMask.test(BLEND_COLOR_OP_DIRTY_BITS[i]))
			f.appendf("BlendColorOp[{}]: {}\n", i, (BlendOp)blendStates[i].colorBlendOp);

		if (stateMask.test(BLEND_COLOR_SRC_FACTOR_DIRTY_BITS[i]))
			f.appendf("BlendColorSrc[{}]: {}\n", i, (BlendFactor)blendStates[i].srcColorBlendFactor);

		if (stateMask.test(BLEND_COLOR_DEST_FACTOR_DIRTY_BITS[i]))
			f.appendf("BlendColorDest[{}]: {}\n", i, (BlendFactor)blendStates[i].destColorBlendFactor);

		if (stateMask.test(BLEND_ALPHA_OP_DIRTY_BITS[i]))
			f.appendf("BlendAlphaOp[{}]: {}\n", i, (BlendOp)blendStates[i].alphaBlendOp);

		if (stateMask.test(BLEND_ALPHA_SRC_FACTOR_DIRTY_BITS[i]))
			f.appendf("BlendAlphaSrc[{}]: {}\n", i, (BlendFactor)blendStates[i].srcAlphaBlendFactor);

		if (stateMask.test(BLEND_ALPHA_DEST_FACTOR_DIRTY_BITS[i]))
			f.appendf("BlendAlphaDest[{}]: {}\n", i, (BlendFactor)blendStates[i].destAlphaBlendFactor);
			
		if (stateMask.test(COLOR_MASK_DIRTY_BITS[i]))
			f.appendf("ColorMask[{}]: {}\n", i, Hex(colorMasks[i]));
	}
}

#define CONFIGURE_VAL(val, flag) \
if (key == #flag##_id) { \
	auto ret = val; \
	if (value.match(ret) != MatchResult::OK) { \
		err.appendf("Unable to parse value for '{}' from '{}'", typeid(ret).name(), value); \
		return false; \
	} \
	mask |= GraphicRenderStatesBit::##flag; \
	val = ret; \
	return true; \
}

#define CONFIGURE_BOOL(val, flag) \
if (key == #flag##_id) { \
	bool ret = false; \
	if (value.match(ret) != MatchResult::OK) { \
		err.appendf("Unable to parse boolean from '{}'", value); \
		return false; \
	} \
	mask |= GraphicRenderStatesBit::##flag; \
	val = ret; \
	return true; \
}

#define CONFIGURE_ENUM(enumType, val, flag) \
if (key == #flag##_id) { \
	enumType ret; \
	if (!GetEnumNameValue(StringID::Find(value), ret)) { \
		err.appendf("Unknown option '{}' for {}", value, #enumType); \
		return false; \
	} \
	mask |= GraphicRenderStatesBit::##flag; \
	val = (int)ret; \
	return true; \
}

bool GraphicsRenderStatesSetup::configure(StringID key, StringView value, IFormatStream& err)
{
	TRACE_INFO("Setting render state '{}' = '{}'", key, value);

	CONFIGURE_BOOL(common.scissorEnabled, ScissorEnabled);

	CONFIGURE_BOOL(common.stencilEnabled, StencilEnabled);
	CONFIGURE_ENUM(CompareOp, common.stencilFrontCompareOp, StencilFrontFunc);
	CONFIGURE_ENUM(StencilOp, common.stencilFrontFailOp, StencilFrontFailOp);
	CONFIGURE_ENUM(StencilOp, common.stencilFrontDepthFailOp, StencilFrontDepthFailOp);
	CONFIGURE_ENUM(StencilOp, common.stencilFrontPassOp, StencilFrontPassOp);
	CONFIGURE_ENUM(CompareOp, common.stencilBackCompareOp, StencilBackFunc);
	CONFIGURE_ENUM(StencilOp, common.stencilBackFailOp, StencilBackFailOp);
	CONFIGURE_ENUM(StencilOp, common.stencilBackDepthFailOp, StencilBackDepthFailOp);
	CONFIGURE_ENUM(StencilOp, common.stencilBackPassOp, StencilBackPassOp);

	CONFIGURE_BOOL(common.depthEnabled, DepthEnabled);
	CONFIGURE_BOOL(common.depthWriteEnabled, DepthWriteEnabled);
	CONFIGURE_ENUM(CompareOp, common.depthCompareOp, DepthFunc);

	CONFIGURE_BOOL(common.depthBiasEnabled, DepthBiasEnabled);
	CONFIGURE_VAL(depthStencilStates.depthBias, DepthBiasValue);
	CONFIGURE_VAL(depthStencilStates.depthBiasSlope, DepthBiasSlope);
	CONFIGURE_VAL(depthStencilStates.depthBiasClamp, DepthBiasClamp);

	CONFIGURE_BOOL(common.cullEnabled, CullEnabled);
	CONFIGURE_ENUM(CullMode, common.cullMode, CullMode);
	CONFIGURE_ENUM(FrontFace, common.cullFrontFace, CullFrontFace);

	CONFIGURE_ENUM(FillMode, common.fillMode, FillMode);

	CONFIGURE_BOOL(common.primitiveRestartEnabled, PrimitiveRestartEnabled);
	CONFIGURE_ENUM(PrimitiveTopology, common.primitiveTopology, PrimitiveTopology);

	CONFIGURE_BOOL(common.alphaToCoverageEnable, AlphaCoverageEnabled);
	CONFIGURE_BOOL(common.alphaToCoverageDitherEnable, AlphaCoverageDitherEnabled);

	CONFIGURE_BOOL(common.blendingEnabled, BlendingEnabled);

	CONFIGURE_VAL(colorMasks[0], ColorMask0);
	CONFIGURE_VAL(colorMasks[1], ColorMask1);
	CONFIGURE_VAL(colorMasks[2], ColorMask2);
	CONFIGURE_VAL(colorMasks[3], ColorMask3);
	CONFIGURE_VAL(colorMasks[4], ColorMask4);
	CONFIGURE_VAL(colorMasks[5], ColorMask5);
	CONFIGURE_VAL(colorMasks[6], ColorMask6);
	CONFIGURE_VAL(colorMasks[7], ColorMask7);

	CONFIGURE_ENUM(BlendOp, blendStates[0].colorBlendOp, BlendColorOp0);
	CONFIGURE_ENUM(BlendOp, blendStates[1].colorBlendOp, BlendColorOp1);
	CONFIGURE_ENUM(BlendOp, blendStates[2].colorBlendOp, BlendColorOp2);
	CONFIGURE_ENUM(BlendOp, blendStates[3].colorBlendOp, BlendColorOp3);
	CONFIGURE_ENUM(BlendOp, blendStates[4].colorBlendOp, BlendColorOp4);
	CONFIGURE_ENUM(BlendOp, blendStates[5].colorBlendOp, BlendColorOp5);
	CONFIGURE_ENUM(BlendOp, blendStates[6].colorBlendOp, BlendColorOp6);
	CONFIGURE_ENUM(BlendOp, blendStates[7].colorBlendOp, BlendColorOp7);

	CONFIGURE_ENUM(BlendFactor, blendStates[0].srcColorBlendFactor, BlendColorSrc0);
	CONFIGURE_ENUM(BlendFactor, blendStates[1].srcColorBlendFactor, BlendColorSrc1);
	CONFIGURE_ENUM(BlendFactor, blendStates[2].srcColorBlendFactor, BlendColorSrc2);
	CONFIGURE_ENUM(BlendFactor, blendStates[3].srcColorBlendFactor, BlendColorSrc3);
	CONFIGURE_ENUM(BlendFactor, blendStates[4].srcColorBlendFactor, BlendColorSrc4);
	CONFIGURE_ENUM(BlendFactor, blendStates[5].srcColorBlendFactor, BlendColorSrc5);
	CONFIGURE_ENUM(BlendFactor, blendStates[6].srcColorBlendFactor, BlendColorSrc6);
	CONFIGURE_ENUM(BlendFactor, blendStates[7].srcColorBlendFactor, BlendColorSrc7);

	CONFIGURE_ENUM(BlendFactor, blendStates[0].destColorBlendFactor, BlendColorDest0);
	CONFIGURE_ENUM(BlendFactor, blendStates[1].destColorBlendFactor, BlendColorDest1);
	CONFIGURE_ENUM(BlendFactor, blendStates[2].destColorBlendFactor, BlendColorDest2);
	CONFIGURE_ENUM(BlendFactor, blendStates[3].destColorBlendFactor, BlendColorDest3);
	CONFIGURE_ENUM(BlendFactor, blendStates[4].destColorBlendFactor, BlendColorDest4);
	CONFIGURE_ENUM(BlendFactor, blendStates[5].destColorBlendFactor, BlendColorDest5);
	CONFIGURE_ENUM(BlendFactor, blendStates[6].destColorBlendFactor, BlendColorDest6);
	CONFIGURE_ENUM(BlendFactor, blendStates[7].destColorBlendFactor, BlendColorDest7);

	CONFIGURE_ENUM(BlendOp, blendStates[0].alphaBlendOp, BlendAlphaOp0);
	CONFIGURE_ENUM(BlendOp, blendStates[1].alphaBlendOp, BlendAlphaOp1);
	CONFIGURE_ENUM(BlendOp, blendStates[2].alphaBlendOp, BlendAlphaOp2);
	CONFIGURE_ENUM(BlendOp, blendStates[3].alphaBlendOp, BlendAlphaOp3);
	CONFIGURE_ENUM(BlendOp, blendStates[4].alphaBlendOp, BlendAlphaOp4);
	CONFIGURE_ENUM(BlendOp, blendStates[5].alphaBlendOp, BlendAlphaOp5);
	CONFIGURE_ENUM(BlendOp, blendStates[6].alphaBlendOp, BlendAlphaOp6);
	CONFIGURE_ENUM(BlendOp, blendStates[7].alphaBlendOp, BlendAlphaOp7);

	CONFIGURE_ENUM(BlendFactor, blendStates[0].srcAlphaBlendFactor, BlendAlphaSrc0);
	CONFIGURE_ENUM(BlendFactor, blendStates[1].srcAlphaBlendFactor, BlendAlphaSrc1);
	CONFIGURE_ENUM(BlendFactor, blendStates[2].srcAlphaBlendFactor, BlendAlphaSrc2);
	CONFIGURE_ENUM(BlendFactor, blendStates[3].srcAlphaBlendFactor, BlendAlphaSrc3);
	CONFIGURE_ENUM(BlendFactor, blendStates[4].srcAlphaBlendFactor, BlendAlphaSrc4);
	CONFIGURE_ENUM(BlendFactor, blendStates[5].srcAlphaBlendFactor, BlendAlphaSrc5);
	CONFIGURE_ENUM(BlendFactor, blendStates[6].srcAlphaBlendFactor, BlendAlphaSrc6);
	CONFIGURE_ENUM(BlendFactor, blendStates[7].srcAlphaBlendFactor, BlendAlphaSrc7);

	CONFIGURE_ENUM(BlendFactor, blendStates[0].destAlphaBlendFactor, BlendAlphaDest0);
	CONFIGURE_ENUM(BlendFactor, blendStates[1].destAlphaBlendFactor, BlendAlphaDest1);
	CONFIGURE_ENUM(BlendFactor, blendStates[2].destAlphaBlendFactor, BlendAlphaDest2);
	CONFIGURE_ENUM(BlendFactor, blendStates[3].destAlphaBlendFactor, BlendAlphaDest3);
	CONFIGURE_ENUM(BlendFactor, blendStates[4].destAlphaBlendFactor, BlendAlphaDest4);
	CONFIGURE_ENUM(BlendFactor, blendStates[5].destAlphaBlendFactor, BlendAlphaDest5);
	CONFIGURE_ENUM(BlendFactor, blendStates[6].destAlphaBlendFactor, BlendAlphaDest6);
	CONFIGURE_ENUM(BlendFactor, blendStates[7].destAlphaBlendFactor, BlendAlphaDest7);

	err.appendf("Unknown render state '{}'", key);
	return false;
}

//--

void GraphicsRenderStatesSetup::reset()
{
	memzero(this, sizeof(GraphicsRenderStatesSetup));

	common.fillMode = (int)FillMode::Fill;
	common.depthEnabled = true;
	common.depthWriteEnabled = true;
	common.depthCompareOp = (int)CompareOp::LessEqual;
	common.stencilFrontFailOp = (int)StencilOp::Keep;
	common.stencilFrontPassOp = (int)StencilOp::Keep;
	common.stencilFrontDepthFailOp = (int)StencilOp::Keep;
	common.stencilFrontCompareOp = (int)CompareOp::Always;
	common.stencilBackFailOp = (int)StencilOp::Keep;
	common.stencilBackPassOp = (int)StencilOp::Keep;
	common.stencilBackDepthFailOp = (int)StencilOp::Keep;
	common.stencilBackCompareOp = (int)CompareOp::Always;
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

	depthStencilStates.stencilReadMask = 0xFF;
	depthStencilStates.stencilWriteMask = 0xFF;
}

void GraphicsRenderStatesSetup::colorMask(uint8_t i, uint8_t colorMask)
{
	DEBUG_CHECK_RETURN(i < GraphicsRenderStatesSetup::MAX_TARGETS);

	mask |= GraphicsRenderStatesSetup::COLOR_MASK_DIRTY_BITS[i];
	colorMasks[i] = colorMask;
}

void GraphicsRenderStatesSetup::blend(bool enabled)
{
	common.blendingEnabled = enabled;
	mask |= GraphicRenderStatesBit::BlendingEnabled;
}

void GraphicsRenderStatesSetup::blendFactor(uint8_t i, BlendFactor src, BlendFactor dest)
{
	blendFactor(i, src, dest, src, dest);
}

void GraphicsRenderStatesSetup::blendFactor(uint8_t i, BlendFactor srcColor, BlendFactor destColor, BlendFactor srcAlpha, BlendFactor destAlpha)
{
	DEBUG_CHECK_RETURN(i < GraphicsRenderStatesSetup::MAX_TARGETS);

	auto& state = blendStates[i];
	state.srcColorBlendFactor = (int)srcColor;
	state.destColorBlendFactor = (int)destColor;
	state.srcAlphaBlendFactor = (int)srcAlpha;
	state.destAlphaBlendFactor = (int)destAlpha;
	mask |= GraphicsRenderStatesSetup::BLEND_COLOR_DEST_FACTOR_DIRTY_BITS[i];
	mask |= GraphicsRenderStatesSetup::BLEND_COLOR_SRC_FACTOR_DIRTY_BITS[i];
	mask |= GraphicsRenderStatesSetup::BLEND_ALPHA_DEST_FACTOR_DIRTY_BITS[i];
	mask |= GraphicsRenderStatesSetup::BLEND_ALPHA_SRC_FACTOR_DIRTY_BITS[i];
}

void GraphicsRenderStatesSetup::blendOp(uint8_t i, BlendOp op)
{
	blendOp(i, op, op);
}

void GraphicsRenderStatesSetup::blendOp(uint8_t i, BlendOp color, BlendOp alpha)
{
	DEBUG_CHECK_RETURN(i < GraphicsRenderStatesSetup::MAX_TARGETS);

	auto& state = blendStates[i];
	state.colorBlendOp = (int)color;
	state.alphaBlendOp = (int)alpha;
	mask |= GraphicsRenderStatesSetup::BLEND_COLOR_OP_DIRTY_BITS[i];
	mask |= GraphicsRenderStatesSetup::BLEND_ALPHA_OP_DIRTY_BITS[i];
}

void GraphicsRenderStatesSetup::cull(bool enabled)
{
	common.cullEnabled = enabled;
	mask |= GraphicRenderStatesBit::CullEnabled;
}

void GraphicsRenderStatesSetup::cullMode(CullMode mode)
{
	common.cullMode = (int)mode;
	mask |= GraphicRenderStatesBit::CullMode;
}

void GraphicsRenderStatesSetup::cullFrontFace(FrontFace mode)
{
	common.cullFrontFace = (int)mode;
	mask |= GraphicRenderStatesBit::CullFrontFace;
}

void GraphicsRenderStatesSetup::fill(FillMode mode)
{
	common.fillMode = (int)mode;
	mask |= GraphicRenderStatesBit::FillMode;
}

void GraphicsRenderStatesSetup::scissor(bool enabled)
{
	common.scissorEnabled = enabled;
	mask |= GraphicRenderStatesBit::ScissorEnabled;
}

void GraphicsRenderStatesSetup::stencil(bool enabled)
{
	common.stencilEnabled = enabled;
	mask |= GraphicRenderStatesBit::StencilEnabled;
}

void GraphicsRenderStatesSetup::stencilAll(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp)
{
	stencilFront(compareOp, failOp, depthFailOp, passOp);
	stencilBack(compareOp, failOp, depthFailOp, passOp);
}

void GraphicsRenderStatesSetup::stencilFront(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp)
{
	common.stencilFrontCompareOp = (int)compareOp;
	common.stencilFrontFailOp = (int)failOp;
	common.stencilFrontPassOp = (int)passOp;
	common.stencilFrontDepthFailOp = (int)depthFailOp;
	mask |= GraphicRenderStatesBit::StencilFrontFunc;
	mask |= GraphicRenderStatesBit::StencilFrontFailOp;
	mask |= GraphicRenderStatesBit::StencilFrontPassOp;
	mask |= GraphicRenderStatesBit::StencilFrontDepthFailOp;
}

void GraphicsRenderStatesSetup::stencilBack(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp)
{
	common.stencilBackCompareOp = (int)compareOp;
	common.stencilBackFailOp = (int)failOp;
	common.stencilBackPassOp = (int)passOp;
	common.stencilBackDepthFailOp = (int)depthFailOp;
	mask |= GraphicRenderStatesBit::StencilBackFunc;
	mask |= GraphicRenderStatesBit::StencilBackFailOp;
	mask |= GraphicRenderStatesBit::StencilBackPassOp;
	mask |= GraphicRenderStatesBit::StencilBackDepthFailOp;
}

void GraphicsRenderStatesSetup::stencilReadMask(uint8_t value)
{
	mask |= GraphicRenderStatesBit::StencilReadMask;
	depthStencilStates.stencilReadMask = value;
}

void GraphicsRenderStatesSetup::stencilWriteMask(uint8_t value)
{
	mask |= GraphicRenderStatesBit::StencilWriteMask;
	depthStencilStates.stencilWriteMask = value;
}

void GraphicsRenderStatesSetup::depth(bool enable)
{
	common.depthEnabled = enable;
	mask |= GraphicRenderStatesBit::DepthEnabled;
}

void GraphicsRenderStatesSetup::depthWrite(bool enable)
{
	common.depthWriteEnabled = enable;
	mask |= GraphicRenderStatesBit::DepthWriteEnabled;
}

void GraphicsRenderStatesSetup::depthFunc(CompareOp func)
{
	common.depthCompareOp = (int)func;
	mask |= GraphicRenderStatesBit::DepthFunc;
}

void GraphicsRenderStatesSetup::depthBias(bool enabled)
{
	common.depthBiasEnabled = enabled;
	mask |= GraphicRenderStatesBit::DepthBiasEnabled;
}

void GraphicsRenderStatesSetup::depthBiasValue(int value)
{
	depthStencilStates.depthBias = value;
	mask |= GraphicRenderStatesBit::DepthBiasValue;
}

void GraphicsRenderStatesSetup::depthBiasSlope(float value)
{
	depthStencilStates.depthBiasSlope = value;
	mask |= GraphicRenderStatesBit::DepthBiasSlope;
}

void GraphicsRenderStatesSetup::depthBiasClamp(float value)
{
	depthStencilStates.depthBiasClamp = value;
	mask |= GraphicRenderStatesBit::DepthBiasClamp;
}

void GraphicsRenderStatesSetup::primitiveTopology(PrimitiveTopology topology)
{
	common.primitiveTopology = (int)topology;
	mask |= GraphicRenderStatesBit::PrimitiveTopology;
}

void GraphicsRenderStatesSetup::primitiveRestart(bool enabled)
{
	common.primitiveRestartEnabled = enabled;
	mask |= GraphicRenderStatesBit::PrimitiveRestartEnabled;
}

void GraphicsRenderStatesSetup::alphaToCoverage(bool enabled)
{
	common.alphaToCoverageEnable = enabled;
	mask |= GraphicRenderStatesBit::AlphaCoverageEnabled;
}

void GraphicsRenderStatesSetup::alphaToCoverageDither(bool enabled)
{
	common.alphaToCoverageDitherEnable = enabled;
	mask |= GraphicRenderStatesBit::AlphaCoverageDitherEnabled;
}

void GraphicsRenderStatesSetup::alphaToOne(bool enabled)
{
	/*if (common.alphaToCoverageDitherEnable != enabled)
	{
		common.alphaToCoverageDitherEnable = enabled;
		mask |= GraphicRenderStatesBit::AlphaCoverageDitherEnabled;
	}*/
}

//--

static const uint8_t WRITE_MASK_STENCIL_MASK = 1;
static const uint8_t WRITE_MASK_BLENDING = 2;
static const uint8_t WRITE_MASK_COLOR_MASK = 4;
static const uint8_t WRITE_MASK_DEPTH_BIAS = 8;
static const uint8_t WRITE_MASK_COMMON_STATES = 16;

static bool IsBlendStateUsed(const GraphicsRenderStatesSetup::BlendState& bs)
{
	if ((BlendOp)bs.colorBlendOp != BlendOp::Add || (BlendOp)bs.alphaBlendOp != BlendOp::Add)
		return true;

	if ((BlendFactor)bs.srcColorBlendFactor != BlendFactor::One || (BlendFactor)bs.srcAlphaBlendFactor != BlendFactor::One)
		return true;

	if ((BlendFactor)bs.destColorBlendFactor != BlendFactor::Zero || (BlendFactor)bs.destAlphaBlendFactor != BlendFactor::Zero)
		return true;

	return false;
}

void GraphicsRenderStatesSetup::writeBinary(stream::OpcodeWriter& stream) const
{
	uint8_t writeMask = 0;
	uint8_t blendingMask = 0;
	uint8_t colorMask = 0;

	stream.writeData(&mask, sizeof(mask));

	const auto& commonStateValues = *(const uint64_t*)&common;
	const auto& defaultCommonStateValues = *(const uint64_t*)&DEFAULT_STATES().common;		
	if (commonStateValues != defaultCommonStateValues)
		writeMask |= WRITE_MASK_COMMON_STATES;

	if (common.depthBiasEnabled)
	{
		if (depthStencilStates.depthBias != 0 || depthStencilStates.depthBiasSlope != 0.0f || depthStencilStates.depthBiasClamp != 0.0)
			writeMask |= WRITE_MASK_DEPTH_BIAS;
	}

	if (common.stencilEnabled)
	{
		if (depthStencilStates.stencilReadMask != 0xFF || depthStencilStates.stencilWriteMask != 0xFF)
			writeMask |= WRITE_MASK_STENCIL_MASK;
	}

	if (common.blendingEnabled)
	{
		for (int i = 0; i < MAX_TARGETS; ++i)
			if (IsBlendStateUsed(blendStates[i]))
				blendingMask |= 1U << i;

		if (blendingMask)
			writeMask |= WRITE_MASK_BLENDING;
	}

	for (int i = 0; i < MAX_TARGETS; ++i)
		if ((colorMasks[i] & 15) != 0x0F)
			colorMask |= 1U << i;

	if (colorMask)
		writeMask |= WRITE_MASK_COLOR_MASK;

	stream.writeTypedData(writeMask);

	if (writeMask & WRITE_MASK_COMMON_STATES)
		stream.writeTypedData(commonStateValues);

	if (writeMask & WRITE_MASK_DEPTH_BIAS)
	{
		stream.writeTypedData(depthStencilStates.depthBias);
		stream.writeTypedData(depthStencilStates.depthBiasSlope);
		stream.writeTypedData(depthStencilStates.depthBiasClamp);
	}

	if (writeMask & WRITE_MASK_STENCIL_MASK)
	{
		stream.writeTypedData(depthStencilStates.stencilReadMask);
		stream.writeTypedData(depthStencilStates.stencilWriteMask);
	}

	if (writeMask & WRITE_MASK_BLENDING)
	{
		stream.writeTypedData(blendingMask);

		for (int i = 0; i < MAX_TARGETS; ++i)
		{
			if (blendingMask & (1 << i))
			{
				const auto& blendStateValue = *(const uint32_t*)&blendStates[i];
				stream.writeTypedData(blendStateValue);
			}
		}
	}

	if (writeMask & WRITE_MASK_COLOR_MASK)
	{
		stream.writeTypedData(colorMask);

		for (int i = 0; i < MAX_TARGETS; ++i)
			if (colorMask & (1 << i))
				stream.writeTypedData(colorMasks[i]);
	}
}

void GraphicsRenderStatesSetup::readBinary(stream::OpcodeReader& stream)
{
	reset();

	stream.readData(&mask, sizeof(mask));

	uint8_t writeMask = 0;
	stream.readTypedData(writeMask);

	if (writeMask & WRITE_MASK_COMMON_STATES)
	{
		auto& commonStateValues = *(uint64_t*)&common;
		stream.readTypedData(commonStateValues);
	}

	if (writeMask & WRITE_MASK_DEPTH_BIAS)
	{
		stream.readTypedData(depthStencilStates.depthBias);
		stream.readTypedData(depthStencilStates.depthBiasSlope);
		stream.readTypedData(depthStencilStates.depthBiasClamp);
	}

	if (writeMask & WRITE_MASK_STENCIL_MASK)
	{
		stream.readTypedData(depthStencilStates.stencilReadMask);
		stream.readTypedData(depthStencilStates.stencilWriteMask);
	}

	if (writeMask & WRITE_MASK_BLENDING)
	{
		uint8_t blendingMask = 0;
		stream.readTypedData(blendingMask);

		for (int i = 0; i < MAX_TARGETS; ++i)
		{
			if (blendingMask & (1 << i))
			{
				auto& blendStateValue = *(uint32_t*)&blendStates[i];
				stream.readTypedData(blendStateValue);
			}
		}
	}

	if (writeMask & WRITE_MASK_COLOR_MASK)
	{
		uint8_t colorMask = 0;
		stream.readTypedData(colorMask);

		for (int i = 0; i < MAX_TARGETS; ++i)
			if (colorMask & (1 << i))
				stream.readTypedData(colorMasks[i]);
	}
}	

END_BOOMER_NAMESPACE_EX(gpu)
