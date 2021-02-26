/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: data #]
***/

#include "build.h"
#include "renderingImageFormat.h"

BEGIN_BOOMER_NAMESPACE()

//---

RTTI_BEGIN_TYPE_ENUM(ImageFormat);
    RTTI_ENUM_OPTION(UNKNOWN);
    RTTI_ENUM_OPTION(R8_SNORM);
    RTTI_ENUM_OPTION(R8_UNORM);
    RTTI_ENUM_OPTION(R8_UINT);
    RTTI_ENUM_OPTION(R8_INT);
    RTTI_ENUM_OPTION(RG8_SNORM);
    RTTI_ENUM_OPTION(RG8_UNORM);
    RTTI_ENUM_OPTION(RG8_UINT);
    RTTI_ENUM_OPTION(RG8_INT);
    RTTI_ENUM_OPTION(RGB8_SNORM);
    RTTI_ENUM_OPTION(RGB8_UNORM);
    RTTI_ENUM_OPTION(RGB8_UINT);
    RTTI_ENUM_OPTION(RGB8_INT);
    RTTI_ENUM_OPTION(RGBA8_UNORM);
    RTTI_ENUM_OPTION(RGBA8_SNORM);
    RTTI_ENUM_OPTION(RGBA8_UINT);
    RTTI_ENUM_OPTION(RGBA8_INT);
    RTTI_ENUM_OPTION(BGRA8_UNORM);
    RTTI_ENUM_OPTION(R16F);
    RTTI_ENUM_OPTION(RG16F);
    RTTI_ENUM_OPTION(RGBA16F);
    RTTI_ENUM_OPTION(R32F);
    RTTI_ENUM_OPTION(RG32F);
    RTTI_ENUM_OPTION(RGB32F);
    RTTI_ENUM_OPTION(RGBA32F);
    RTTI_ENUM_OPTION(R16_UNORM);
    RTTI_ENUM_OPTION(R16_SNORM);
    RTTI_ENUM_OPTION(R16_UINT);
    RTTI_ENUM_OPTION(R16_INT);
    RTTI_ENUM_OPTION(RG16_UNORM);
    RTTI_ENUM_OPTION(RG16_SNORM);
    RTTI_ENUM_OPTION(RG16_UINT);
    RTTI_ENUM_OPTION(RG16_INT);
    RTTI_ENUM_OPTION(RGBA16_UNORM);
    RTTI_ENUM_OPTION(RGBA16_SNORM);
    RTTI_ENUM_OPTION(RGBA16_UINT);
    RTTI_ENUM_OPTION(RGBA16_INT);
    RTTI_ENUM_OPTION(R32_UINT);
    RTTI_ENUM_OPTION(R32_INT);
    RTTI_ENUM_OPTION(RG32_UINT);
    RTTI_ENUM_OPTION(RG32_INT);
    RTTI_ENUM_OPTION(RGBA32_UINT);
    RTTI_ENUM_OPTION(RGBA32_INT);
    RTTI_ENUM_OPTION(MAT22F);
    RTTI_ENUM_OPTION(MAT32F);
    RTTI_ENUM_OPTION(MAT33F);
    RTTI_ENUM_OPTION(MAT42F);
    RTTI_ENUM_OPTION(MAT43F);
    RTTI_ENUM_OPTION(MAT44F);
    RTTI_ENUM_OPTION(R11FG11FB10F);
    RTTI_ENUM_OPTION(RGB10_A2_UNORM);
    RTTI_ENUM_OPTION(RGB10_A2_UINT);
    RTTI_ENUM_OPTION(RGBA4_UNORM);
    RTTI_ENUM_OPTION(BC1_UNORM);
    RTTI_ENUM_OPTION(BC2_UNORM);
    RTTI_ENUM_OPTION(BC3_UNORM);
    RTTI_ENUM_OPTION(BC4_UNORM);
    RTTI_ENUM_OPTION(BC5_UNORM);
    RTTI_ENUM_OPTION(BC6_UNSIGNED);
    RTTI_ENUM_OPTION(BC6_SIGNED);
    RTTI_ENUM_OPTION(BC7_UNORM);
    RTTI_ENUM_OPTION(SRGB8);
    RTTI_ENUM_OPTION(SRGBA8);
    RTTI_ENUM_OPTION(BC1_SRGB);
    RTTI_ENUM_OPTION(BC2_SRGB);
    RTTI_ENUM_OPTION(BC3_SRGB);
    RTTI_ENUM_OPTION(BC7_SRGB);
    RTTI_ENUM_OPTION(D24S8);
    RTTI_ENUM_OPTION(D24FS8);
    RTTI_ENUM_OPTION(D32);
RTTI_END_TYPE();

//---

	//--

static ImageFormatInfo GImageFormatInfos[(uint8_t)ImageFormat::MAX] = {
	{ "UNKNOWN", "", CompileTimeCRC32("UNKNOWN").crc(), 0, 0, false, false, ImageFormatClass::UNORM },
	{ "R8_SNORM", "r8_snorm", CompileTimeCRC32("R8_SNORM").crc(), 1, 8, false, false, ImageFormatClass::SNORM },
	{ "R8_UNORM", "r8", CompileTimeCRC32("R8_UNORM").crc(), 1, 8, false, false, ImageFormatClass::UNORM },
	{ "R8_UINT", "r8ui", CompileTimeCRC32("R8_UINT").crc(), 1, 8, false, false, ImageFormatClass::UINT },
	{ "R8_INT", "r8i", CompileTimeCRC32("R8_INT").crc(), 1, 8, false, false, ImageFormatClass::INT },
	{ "RG8_SNORM", "rg8_snorm", CompileTimeCRC32("RG8_SNORM").crc(), 2, 16, false, false, ImageFormatClass::SNORM },
	{ "RG8_UNORM", "rg8", CompileTimeCRC32("RG8_UNORM").crc(), 2, 16, false, false, ImageFormatClass::UNORM },
	{ "RG8_UINT", "rg8ui", CompileTimeCRC32("RG8_UINT").crc(), 2, 16, false, false, ImageFormatClass::UINT },
	{ "RG8_INT", "rg8i", CompileTimeCRC32("RG8_INT").crc(), 2, 16, false, false, ImageFormatClass::INT },
	{ "RGB8_SNORM", "rgb8_snorm", CompileTimeCRC32("RGB8_SNORM").crc(), 3, 24, false, false, ImageFormatClass::SNORM },
	{ "RGB8_UNORM", "rgb8", CompileTimeCRC32("RGB8_UNORM").crc(), 3, 24, false, false, ImageFormatClass::UNORM },
	{ "RGB8_UINT", "rgb8ui", CompileTimeCRC32("RGB8_UINT").crc(), 3, 24, false, false, ImageFormatClass::UINT },
	{ "RGB8_INT", "rgb8i", CompileTimeCRC32("RGB8_INT").crc(), 3, 24, false, false, ImageFormatClass::INT },
	{ "RGBA8_SNORM", "rgba8_snorm", CompileTimeCRC32("RGBA8_SNORM").crc(), 4, 32, false, false, ImageFormatClass::SNORM },
	{ "RGBA8_UNORM", "rgba8", CompileTimeCRC32("RGBA8_UNORM").crc(), 4, 32, false, false, ImageFormatClass::UNORM },
	{ "RGBA8_UINT", "rgba8ui", CompileTimeCRC32("RGBA8_UINT").crc(), 4, 32, false, false, ImageFormatClass::UINT },
	{ "RGBA8_INT", "rgba8i", CompileTimeCRC32("RGBA8_INT").crc(), 4, 32, false, false, ImageFormatClass::INT },
	{ "BGRA8_UNORM", "bgra8", CompileTimeCRC32("BGRA8_UNORM").crc(), 4, 32, false, false, ImageFormatClass::UNORM },

	{ "R16F", "r16f", CompileTimeCRC32("R16F").crc(), 1, 16, false, false, ImageFormatClass::FLOAT },
	{ "RG16F", "rg16f", CompileTimeCRC32("RG16F").crc(), 2, 32, false, false, ImageFormatClass::FLOAT },
	{ "RGBA16F", "rgba16f", CompileTimeCRC32("RGBA16F").crc(), 4, 64, false, false, ImageFormatClass::FLOAT },
	{ "R32F", "r32f", CompileTimeCRC32("R32F").crc(), 1, 32, false, false, ImageFormatClass::FLOAT },
	{ "RG32F", "rg32f", CompileTimeCRC32("RG32F").crc(), 2, 64, false, false, ImageFormatClass::FLOAT },
	{ "RGB32F", "rgb32f", CompileTimeCRC32("RGB32F").crc(), 3, 96, false, false, ImageFormatClass::FLOAT },
	{ "RGBA32F", "rgba32f", CompileTimeCRC32("RGBA32F").crc(), 4, 128, false, false, ImageFormatClass::FLOAT },

	{ "R16_INT", "r16i", CompileTimeCRC32("R16_INT").crc(), 1, 16, false, false, ImageFormatClass::INT },
	{ "R16_UINT", "r16ui", CompileTimeCRC32("R16_UINT").crc(), 1, 16, false, false, ImageFormatClass::UINT },
	{ "R16_UNORM", "r16", CompileTimeCRC32("R16_UNORM").crc(), 1, 16, false, false, ImageFormatClass::UNORM },
	{ "R16_SNORM", "r16_snorm", CompileTimeCRC32("R16_SNORM").crc(), 1, 16, false, false, ImageFormatClass::SNORM },
	{ "RG16_INT", "rg16i", CompileTimeCRC32("RG16_INT").crc(), 2, 32, false, false, ImageFormatClass::INT },
	{ "RG16_UINT", "rg16ui", CompileTimeCRC32("RG16_UINT").crc(), 2, 32, false, false, ImageFormatClass::UINT },
	{ "RG16_UNORM", "rg16", CompileTimeCRC32("RG16_UNORM").crc(), 2, 32, false, false, ImageFormatClass::UNORM },
	{ "RG16_SNORM", "rg16_snorm", CompileTimeCRC32("RG16_SNORM").crc(), 2, 32, false, false, ImageFormatClass::SNORM },
	{ "RGBA16_INT", "rgba16i", CompileTimeCRC32("RGBA16_INT").crc(), 4, 64, false, false, ImageFormatClass::INT },
	{ "RGBA16_UINT", "rgba16ui", CompileTimeCRC32("RGBA16_UINT").crc(), 4, 64, false, false, ImageFormatClass::UINT },
	{ "RGBA16_UNORM", "rgba16", CompileTimeCRC32("RGBA16_UNORM").crc(), 4, 64, false, false, ImageFormatClass::UNORM },
	{ "RGBA16_SNORM", "rgba16_snorm", CompileTimeCRC32("RGBA16_SNORM").crc(), 4, 64, false, false, ImageFormatClass::SNORM },

	{ "R32_INT", "r32i", CompileTimeCRC32("R32_INT").crc(), 1, 32, false, false, ImageFormatClass::INT },
	{ "R32_UINT", "r32ui", CompileTimeCRC32("R32_UINT").crc(), 1, 32, false, false, ImageFormatClass::UINT },
	{ "RG32_INT", "rg32i", CompileTimeCRC32("RG32_INT").crc(), 2, 64, false, false, ImageFormatClass::INT },
	{ "RG32_UINT", "rg32ui", CompileTimeCRC32("RG32_UINT").crc(), 2, 64, false, false, ImageFormatClass::UINT },
	{ "RGB32_INT", "rgb32i", CompileTimeCRC32("RGB32_INT").crc(), 3, 96, false, false, ImageFormatClass::INT },
	{ "RGB32_UINT", "rgb32ui", CompileTimeCRC32("RGB32_UINT").crc(), 3, 96, false, false, ImageFormatClass::UINT },
	{ "RGBA32_INT", "rgba32i", CompileTimeCRC32("RGBA32_INT").crc(), 4, 128, false, false, ImageFormatClass::INT },
	{ "RGBA32_UINT", "rgba32ui", CompileTimeCRC32("RGBA32_UINT").crc(), 4, 128, false, false, ImageFormatClass::UINT },

	{ "MAT22F", "mat22", CompileTimeCRC32("MAT22").crc(), 4, 32 * 4, true, false, ImageFormatClass::FLOAT },
	{ "MAT32F", "mat32", CompileTimeCRC32("MAT32").crc(), 6, 32 * 6, true, false, ImageFormatClass::FLOAT },
	{ "MAT33F", "mat33", CompileTimeCRC32("MAT33").crc(), 9, 32 * 9, true, false, ImageFormatClass::FLOAT },
	{ "MAT42F", "mat42", CompileTimeCRC32("MAT42").crc(), 8, 32 * 8, true, false, ImageFormatClass::FLOAT },
	{ "MAT43F", "mat43", CompileTimeCRC32("MAT43").crc(), 12, 32 * 12, true, false, ImageFormatClass::FLOAT },
	{ "MAT44F", "mat44", CompileTimeCRC32("MAT44").crc(), 16, 32 * 16, true, false, ImageFormatClass::FLOAT },

	{ "R11FG11FB10F", "r11f_g11f_b10f", CompileTimeCRC32("R11FG11FB10F").crc(), 3, 32, false, true, ImageFormatClass::FLOAT },
	{ "RGB10_A2_UNORM", "rgb10_a2", CompileTimeCRC32("RGB10_A2_UNORM").crc(), 4, 32, false, true, ImageFormatClass::UNORM },
	{ "RGB10_A2_UINT", "rgb10_a2ui", CompileTimeCRC32("RGB10_A2_UINT").crc(), 4, 32, false, true, ImageFormatClass::UINT },
	{ "RGBA4_UNORM", "rgba4", CompileTimeCRC32("RGBA4_UNORM").crc(), 4, 16, false, true, ImageFormatClass::UNORM },

	{ "BC1_UNORM", "bc1", CompileTimeCRC32("BC1_UNORM").crc(), 4, 4, true, true, ImageFormatClass::UNORM },
	{ "BC2_UNORM", "bc2", CompileTimeCRC32("BC2_UNORM").crc(), 4, 8, true, true, ImageFormatClass::UNORM },
	{ "BC3_UNORM", "bc3", CompileTimeCRC32("BC3_UNORM").crc(), 4, 8, true, true, ImageFormatClass::UNORM },
	{ "BC4_UNORM", "bc4", CompileTimeCRC32("BC4_UNORM").crc(), 1, 4, true, true, ImageFormatClass::UNORM },
	{ "BC5_UNORM", "bc5", CompileTimeCRC32("BC5_UNORM").crc(), 2, 8, true, true, ImageFormatClass::UNORM },
	{ "BC6_UNSIGNED", "bc6ui", CompileTimeCRC32("BC6_UINT").crc(), 2, 8, true, true, ImageFormatClass::FLOAT },
	{ "BC6_SIGNED", "bc6i", CompileTimeCRC32("BC6_INT").crc(), 2, 8, true, true, ImageFormatClass::FLOAT },
	{ "BC7_UNORM", "bc7", CompileTimeCRC32("BC7_UNORM").crc(), 4, 8, true, true, ImageFormatClass::UNORM },

	{ "SRGB8", "srgb8", CompileTimeCRC32("SRGB8").crc(), 3, 24, false, false, ImageFormatClass::SRGB },
	{ "SRGBA8", "srgba8", CompileTimeCRC32("SRGBA8").crc(), 4, 32, false, false, ImageFormatClass::SRGB },
	{ "BC1_SRGB", "bc1_srgb", CompileTimeCRC32("BC1_SRGB").crc(), 4, 4, true, true, ImageFormatClass::SRGB },
	{ "BC2_SRGB", "bc2_srgb", CompileTimeCRC32("BC2_SRGB").crc(), 4, 8, true, true, ImageFormatClass::SRGB },
	{ "BC3_SRGB", "bc3_srgb", CompileTimeCRC32("BC3_SRGB").crc(), 4, 8, true, true, ImageFormatClass::SRGB },
	{ "BC7_SRGB", "bc7_srgb", CompileTimeCRC32("BC7_SRGB").crc(), 4, 8, true, true, ImageFormatClass::SRGB },

	{ "D24S8", "d24s8", CompileTimeCRC32("D24S8").crc(), 2, 32, false, true, ImageFormatClass::DEPTH },
	{ "D24FS8", "d24fs8", CompileTimeCRC32("D24FS8").crc(), 2, 32, false, true, ImageFormatClass::DEPTH },
	{ "D32", "d32", CompileTimeCRC32("D32").crc(), 1, 32, false, false, ImageFormatClass::DEPTH },
};

//--

const ImageFormatInfo& GetImageFormatInfo(ImageFormat format)
{
	auto index = (uint32_t)format;
	if (index >= (uint32_t)ImageFormat::MAX)
		return GImageFormatInfos[0];

#ifdef BUILD_DEBUG
	auto name = reflection::GetEnumValueName<ImageFormat>(format);
	auto formatName = GImageFormatInfos[index].name;
	DEBUG_CHECK(0 == strcmp(name, formatName));
#endif

	return GImageFormatInfos[index];
}

bool GetImageFormatByDisplayName(StringView name, ImageFormat& outFormat)
{
	for (uint32_t i = 1; i < (uint8_t)ImageFormat::MAX; ++i)
	{
		if (name == GImageFormatInfos[i].name)
		{
			outFormat = (ImageFormat)i;
			return true;
		}
	}

	return false;
}

bool GetImageFormatByShaderName(StringView name, ImageFormat& outFormat)
{
	for (uint32_t i = 1; i < (uint8_t)ImageFormat::MAX; ++i)
	{
		if (name == GImageFormatInfos[i].shaderName)
		{
			outFormat = (ImageFormat)i;
			return true;
		}
	}

	return false;
}

//--

bool IsFormatValidForView(ImageFormat format)
{
	switch (format)
	{
	case ImageFormat::RGBA32F:
	case ImageFormat::RGBA16F:
	case ImageFormat::RG32F:
	case ImageFormat::RG16F:
	case ImageFormat::R11FG11FB10F:
	case ImageFormat::R32F:
	case ImageFormat::R16F:
	case ImageFormat::RGBA16_UNORM:
	case ImageFormat::RGB10_A2_UNORM:
	case ImageFormat::RGBA8_UNORM:
	case ImageFormat::RG16_UNORM:
	case ImageFormat::RG8_UNORM:
	case ImageFormat::R16_UNORM:
	case ImageFormat::R8_UNORM:
	case ImageFormat::RGBA16_SNORM:
	case ImageFormat::RGBA8_SNORM:
	case ImageFormat::RG16_SNORM:
	case ImageFormat::RG8_SNORM:
	case ImageFormat::R16_SNORM:
	case ImageFormat::R8_SNORM:

	case ImageFormat::RGBA32_INT:
	case ImageFormat::RGBA16_INT:
	case ImageFormat::RGBA8_INT:
	case ImageFormat::RG32_INT:
	case ImageFormat::RG16_INT:
	case ImageFormat::RG8_INT:
	case ImageFormat::R32_INT:
	case ImageFormat::R16_INT:
	case ImageFormat::R8_INT:

	case ImageFormat::RGBA32_UINT:
	case ImageFormat::RGBA16_UINT:
	case ImageFormat::RGB10_A2_UINT:
	case ImageFormat::RGBA8_UINT:
	case ImageFormat::RG16_UINT:
	case ImageFormat::RG8_UINT:
	case ImageFormat::R32_UINT:
	case ImageFormat::R16_UINT:
	case ImageFormat::R8_UINT:
		return true;
	}

	return false;
}

bool IsFormatValidForAtomic(ImageFormat format)
{
	switch (format)
	{
	case ImageFormat::R32_INT:
	case ImageFormat::R32_UINT:
		return true;
	}

	return false;
}

END_BOOMER_NAMESPACE()
