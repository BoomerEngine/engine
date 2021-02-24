/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: data #]
***/

#pragma once

#include "renderingGraphicsStates.h"

BEGIN_BOOMER_NAMESPACE(rendering)
	
//--

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
    Clamp,
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

//--

/// sampler state
struct RENDERING_DEVICE_API SamplerState
{
	RTTI_DECLARE_NONVIRTUAL_CLASS(SamplerState);

public:
    FilterMode magFilter = FilterMode::Nearest;
    FilterMode minFilter = FilterMode::Nearest;
    MipmapFilterMode mipmapMode = MipmapFilterMode::Nearest;

    AddressMode addresModeU = AddressMode::Clamp;
    AddressMode addresModeV = AddressMode::Clamp;
    AddressMode addresModeW = AddressMode::Clamp;

	bool compareEnabled = false;
	CompareOp compareOp = CompareOp::LessEqual;

	BorderColor borderColor = BorderColor::FloatTransparentBlack;

	uint8_t maxAnisotropy = 0; // unlimited

    float mipLodBias = 0.0f;
	float minLod = 0.0f;
	float maxLod = 16.0f;

	//--

	base::StringBuf label; 

	//--

    static uint32_t CalcHash(const SamplerState& key);

    bool operator==(const SamplerState& other) const;
    bool operator!=(const SamplerState& other) const;

	void print(base::IFormatStream& f) const;

	bool configure(base::StringID key, base::StringView value, base::IFormatStream& err);
};

//--

END_BOOMER_NAMESPACE(rendering)
