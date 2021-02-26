/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: data #]
***/

#include "build.h"
#include "renderingSamplerState.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu)

//---

RTTI_BEGIN_TYPE_ENUM(FilterMode);
    RTTI_ENUM_OPTION(Nearest);
    RTTI_ENUM_OPTION(Linear);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(MipmapFilterMode);
	RTTI_ENUM_OPTION(None);
    RTTI_ENUM_OPTION(Nearest);
    RTTI_ENUM_OPTION(Linear);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_ENUM(AddressMode);
    RTTI_ENUM_OPTION(Wrap);
    RTTI_ENUM_OPTION(Mirror);
    RTTI_ENUM_OPTION(Clamp);
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

//---

RTTI_BEGIN_TYPE_STRUCT(SamplerState);
	RTTI_BIND_NATIVE_CTOR_DTOR(SamplerState);
	RTTI_BIND_NATIVE_COPY(SamplerState);
	RTTI_BIND_NATIVE_COMPARE(SamplerState);
	RTTI_CATEGORY("Filtering");
	RTTI_PROPERTY(magFilter).editable();
	RTTI_PROPERTY(minFilter).editable();
	RTTI_PROPERTY(mipmapMode).editable();
	RTTI_PROPERTY(maxAnisotropy).editable();
	RTTI_CATEGORY("Adressing");
	RTTI_PROPERTY(addresModeU).editable();
	RTTI_PROPERTY(addresModeV).editable();
	RTTI_PROPERTY(addresModeW).editable();
	RTTI_CATEGORY("Bias");
	RTTI_PROPERTY(mipLodBias).editable();
	RTTI_CATEGORY("Limits");
	RTTI_PROPERTY(minLod).editable();
	RTTI_PROPERTY(maxLod).editable();
	RTTI_CATEGORY("Depth compare");
	RTTI_PROPERTY(compareEnabled).editable();
	RTTI_PROPERTY(compareOp).editable();
	RTTI_CATEGORY("Border");
	RTTI_PROPERTY(borderColor).editable();
RTTI_END_TYPE();

uint32_t SamplerState::CalcHash(const SamplerState& key)
{
    CRC64 crc;
    crc << (uint8_t)key.magFilter;
    crc << (uint8_t)key.minFilter;
    crc << (uint8_t)key.mipmapMode;
    crc << (uint8_t)key.addresModeU;
    crc << (uint8_t)key.addresModeV;
    crc << (uint8_t)key.addresModeW;
	crc << (int)(key.mipLodBias * 16.0f);
    crc << key.maxAnisotropy;
    crc << key.compareEnabled;
    if (key.compareEnabled)
        crc << (uint8_t)key.compareOp;
	crc << (int)(key.minLod * 16.0f);
    crc << (int)(key.maxLod * 16.0f);
    crc << (uint8_t)key.borderColor;
    return crc.crc();
}

bool SamplerState::operator==(const SamplerState& other) const
{
    return (magFilter == other.magFilter) && (minFilter == other.minFilter) && (mipmapMode == other.mipmapMode)
        && (addresModeU == other.addresModeU) && (addresModeV == other.addresModeV) && (addresModeW == other.addresModeW)
        && (compareOp == other.compareOp) && (borderColor == other.borderColor)
        && (mipLodBias == other.mipLodBias) && (minLod == other.minLod) && (maxLod == other.maxLod)
        && (maxAnisotropy == other.maxAnisotropy) && (compareEnabled == other.compareEnabled);
}

bool SamplerState::operator!=(const SamplerState& other) const
{
    return !operator==(other);
}

void SamplerState::print(IFormatStream& f) const
{
	f.appendf("Filtering: mag={}, min={}, mip={}, aniso={}\n", magFilter, minFilter, mipmapMode, maxAnisotropy);
	f.appendf("LOD: bias={}, min={}, max={}\n", mipLodBias, minLod, maxLod);
	f.appendf("Addressing: u={}, v={}, w={}", addresModeU, addresModeV, addresModeW);
	f.appendf("Compare: {} {}", compareEnabled, compareOp);
	f.appendf("Border: {}", borderColor);
}

#define CONFIGURE_VAL(val, flag) \
if (key == flag) { \
	auto ret = val; \
	if (value.match(ret) != MatchResult::OK) { \
		err.appendf("Unable to parse value from '{}'", value); \
		return false; \
	} \
	val = ret; \
	return true; \
}

#define CONFIGURE_ENUM(enumType, val, flag) \
if (key == flag) { \
	enumType ret; \
	if (!reflection::GetEnumNameValue(StringID::Find(value), ret)) { \
		err.appendf("Unknown option '{}' for {}", value, #enumType); \
		return false; \
	} \
	val = ret; \
	return true; \
}

bool SamplerState::configure(StringID key, StringView value, IFormatStream& err)
{
	CONFIGURE_ENUM(FilterMode, magFilter, "MagFilter");
	CONFIGURE_ENUM(FilterMode, minFilter, "MinFilter");
	CONFIGURE_ENUM(MipmapFilterMode, mipmapMode, "MipmapMode");
	CONFIGURE_ENUM(MipmapFilterMode, mipmapMode, "MipFilter");

	CONFIGURE_ENUM(AddressMode, addresModeU, "AddressU");
	CONFIGURE_ENUM(AddressMode, addresModeV, "AddressV");
	CONFIGURE_ENUM(AddressMode, addresModeW, "AddressW");

	CONFIGURE_VAL(compareEnabled, "CompareEnabled");
	CONFIGURE_ENUM(CompareOp, compareOp, "CompareOp");

	CONFIGURE_ENUM(BorderColor, borderColor, "BorderColor");

	CONFIGURE_VAL(maxAnisotropy, "MaxAnisotropy");

	CONFIGURE_VAL(mipLodBias, "MipBias");
	CONFIGURE_VAL(minLod, "MinLOD");
	CONFIGURE_VAL(minLod, "MinLod");

	CONFIGURE_VAL(maxLod, "MaxLOD");
	CONFIGURE_VAL(maxLod, "MaxLod");

	err.appendf("Unknown sampler parameter '{}'", key);
	return false;
}

//--

END_BOOMER_NAMESPACE_EX(gpu)
