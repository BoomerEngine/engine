/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface #]
***/

#include "build.h"
#include "renderingDriver.h"

namespace rendering
{

    //--

    RTTI_BEGIN_TYPE_CLASS(DriverNameMetadata);
    RTTI_END_TYPE();

    DriverNameMetadata::DriverNameMetadata()
        : m_name(nullptr)
    {}

    //--

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDriver);
    RTTI_END_TYPE();

    IDriver::IDriver()
    {}

    IDriver::~IDriver()
    {}

    //--

    static ImageFormatInfo GImageFormatInfos[(uint8_t)ImageFormat::MAX] = {
        { "UNKNOWN", "", base::CompileTimeCRC32("UNKNOWN").crc(), 0, 0, false, false, ImageFormatClass::UNORM },
        { "R8_SNORM", "r8_snorm", base::CompileTimeCRC32("R8_SNORM").crc(), 1, 8, false, false, ImageFormatClass::SNORM },
        { "R8_UNORM", "r8", base::CompileTimeCRC32("R8_UNORM").crc(), 1, 8, false, false, ImageFormatClass::UNORM },
        { "R8_UINT", "r8ui", base::CompileTimeCRC32("R8_UINT").crc(), 1, 8, false, false, ImageFormatClass::UINT },
        { "R8_INT", "r8i", base::CompileTimeCRC32("R8_INT").crc(), 1, 8, false, false, ImageFormatClass::INT },
        { "RG8_SNORM", "rg8_snorm", base::CompileTimeCRC32("RG8_SNORM").crc(), 2, 16, false, false, ImageFormatClass::SNORM },
        { "RG8_UNORM", "rg8", base::CompileTimeCRC32("RG8_UNORM").crc(), 2, 16, false, false, ImageFormatClass::UNORM },
        { "RG8_UINT", "rg8ui", base::CompileTimeCRC32("RG8_UINT").crc(), 2, 16, false, false, ImageFormatClass::UINT },
        { "RG8_INT", "rg8i", base::CompileTimeCRC32("RG8_INT").crc(), 2, 16, false, false, ImageFormatClass::INT },
        { "RGB8_SNORM", "rgb8_snorm", base::CompileTimeCRC32("RGB8_SNORM").crc(), 3, 24, false, false, ImageFormatClass::SNORM },
        { "RGB8_UNORM", "rgb8", base::CompileTimeCRC32("RGB8_UNORM").crc(), 3, 24, false, false, ImageFormatClass::UNORM },
        { "RGB8_UINT", "rgb8ui", base::CompileTimeCRC32("RGB8_UINT").crc(), 3, 24, false, false, ImageFormatClass::UINT },
        { "RGB8_INT", "rgb8i", base::CompileTimeCRC32("RGB8_INT").crc(), 3, 24, false, false, ImageFormatClass::INT },
        { "RGBA8_SNORM", "rgba8_snorm", base::CompileTimeCRC32("RGBA8_SNORM").crc(), 4, 32, false, false, ImageFormatClass::SNORM },
        { "RGBA8_UNORM", "rgba8", base::CompileTimeCRC32("RGBA8_UNORM").crc(), 4, 32, false, false, ImageFormatClass::UNORM },
        { "RGBA8_UINT", "rgba8ui", base::CompileTimeCRC32("RGBA8_UINT").crc(), 4, 32, false, false, ImageFormatClass::UINT },
        { "RGBA8_INT", "rgba8i", base::CompileTimeCRC32("RGBA8_INT").crc(), 4, 32, false, false, ImageFormatClass::INT },
        { "BGRA8_UNORM", "bgra8", base::CompileTimeCRC32("BGRA8_UNORM").crc(), 4, 32, false, false, ImageFormatClass::UNORM },

        { "R16F", "r16f", base::CompileTimeCRC32("R16F").crc(), 1, 16, false, false, ImageFormatClass::FLOAT },
        { "RG16F", "rg16f", base::CompileTimeCRC32("RG16F").crc(), 2, 32, false, false, ImageFormatClass::FLOAT },
        { "RGBA16F", "rgba16f", base::CompileTimeCRC32("RGBA16F").crc(), 4, 64, false, false, ImageFormatClass::FLOAT },
        { "R32F", "r32f", base::CompileTimeCRC32("R32F").crc(), 1, 32, false, false, ImageFormatClass::FLOAT },
        { "RG32F", "rg32f", base::CompileTimeCRC32("RG32F").crc(), 2, 64, false, false, ImageFormatClass::FLOAT },
        { "RGB32F", "rgb32f", base::CompileTimeCRC32("RGB32F").crc(), 3, 96, false, false, ImageFormatClass::FLOAT },
        { "RGBA32F", "rgba32f", base::CompileTimeCRC32("RGBA32F").crc(), 4, 128, false, false, ImageFormatClass::FLOAT },

        { "R16_INT", "r16i", base::CompileTimeCRC32("R16_INT").crc(), 1, 16, false, false, ImageFormatClass::INT },
        { "R16_UINT", "r16ui", base::CompileTimeCRC32("R16_UINT").crc(), 1, 16, false, false, ImageFormatClass::UINT },
        { "R16_UNORM", "r16", base::CompileTimeCRC32("R16_UNORM").crc(), 1, 16, false, false, ImageFormatClass::UNORM },
        { "R16_SNORM", "r16_snorm", base::CompileTimeCRC32("R16_SNORM").crc(), 1, 16, false, false, ImageFormatClass::SNORM },
        { "RG16_INT", "rg16i", base::CompileTimeCRC32("RG16_INT").crc(), 2, 32, false, false, ImageFormatClass::INT },
        { "RG16_UINT", "rg16ui", base::CompileTimeCRC32("RG16_UINT").crc(), 2, 32, false, false, ImageFormatClass::UINT },
        { "RG16_UNORM", "rg16", base::CompileTimeCRC32("RG16_UNORM").crc(), 2, 32, false, false, ImageFormatClass::UNORM },
        { "RG16_SNORM", "rg16_snorm", base::CompileTimeCRC32("RG16_SNORM").crc(), 2, 32, false, false, ImageFormatClass::SNORM },
        { "RGBA16_INT", "rgba16i", base::CompileTimeCRC32("RGBA16_INT").crc(), 4, 64, false, false, ImageFormatClass::INT },
        { "RGBA16_UINT", "rgba16ui", base::CompileTimeCRC32("RGBA16_UINT").crc(), 4, 64, false, false, ImageFormatClass::UINT },
        { "RGBA16_UNORM", "rgba16", base::CompileTimeCRC32("RGBA16_UNORM").crc(), 4, 64, false, false, ImageFormatClass::UNORM },
        { "RGBA16_SNORM", "rgba16_snorm", base::CompileTimeCRC32("RGBA16_SNORM").crc(), 4, 64, false, false, ImageFormatClass::SNORM },

        { "R32_INT", "r32i", base::CompileTimeCRC32("R32_INT").crc(), 1, 32, false, false, ImageFormatClass::INT },
        { "R32_UINT", "r32ui", base::CompileTimeCRC32("R32_UINT").crc(), 1, 32, false, false, ImageFormatClass::UINT },
        { "RG32_INT", "rg32i", base::CompileTimeCRC32("RG32_INT").crc(), 2, 64, false, false, ImageFormatClass::INT },
        { "RG32_UINT", "rg32ui", base::CompileTimeCRC32("RG32_UINT").crc(), 2, 64, false, false, ImageFormatClass::UINT },
        { "RGB32_INT", "rgb32i", base::CompileTimeCRC32("RGB32_INT").crc(), 3, 96, false, false, ImageFormatClass::INT },
        { "RGB32_UINT", "rgb32ui", base::CompileTimeCRC32("RGB32_UINT").crc(), 3, 96, false, false, ImageFormatClass::UINT },
        { "RGBA32_INT", "rgba32i", base::CompileTimeCRC32("RGBA32_INT").crc(), 4, 128, false, false, ImageFormatClass::INT },
        { "RGBA32_UINT", "rgba32ui", base::CompileTimeCRC32("RGBA32_UINT").crc(), 4, 128, false, false, ImageFormatClass::UINT },

        { "MAT22F", "mat22", base::CompileTimeCRC32("MAT22").crc(), 4, 32 * 4, true, false, ImageFormatClass::FLOAT },
        { "MAT32F", "mat32", base::CompileTimeCRC32("MAT32").crc(), 6, 32 * 6, true, false, ImageFormatClass::FLOAT },
        { "MAT33F", "mat33", base::CompileTimeCRC32("MAT33").crc(), 9, 32 * 9, true, false, ImageFormatClass::FLOAT },
        { "MAT42F", "mat42", base::CompileTimeCRC32("MAT42").crc(), 8, 32*8, true, false, ImageFormatClass::FLOAT },
        { "MAT43F", "mat43", base::CompileTimeCRC32("MAT43").crc(), 12, 32*12, true, false, ImageFormatClass::FLOAT },
        { "MAT44F", "mat44", base::CompileTimeCRC32("MAT44").crc(), 16, 32*16, true, false, ImageFormatClass::FLOAT },

        { "R11FG11FB10F", "r11f_g11f_b10f", base::CompileTimeCRC32("R11FG11FB10F").crc(), 3, 32, false, true, ImageFormatClass::FLOAT },
        { "RGB10_A2_UNORM", "rgb10_a2", base::CompileTimeCRC32("RGB10_A2_UNORM").crc(), 4, 32, false, true, ImageFormatClass::UNORM },
        { "RGB10_A2_UINT", "rgb10_a2ui", base::CompileTimeCRC32("RGB10_A2_UINT").crc(), 4, 32, false, true, ImageFormatClass::UINT },
        { "RGBA4_UNORM", "rgba4", base::CompileTimeCRC32("RGBA4_UNORM").crc(), 4, 16, false, true, ImageFormatClass::UNORM },

        { "BC1_UNORM", "bc1", base::CompileTimeCRC32("BC1_UNORM").crc(), 4, 4, true, true, ImageFormatClass::UNORM },
        { "BC2_UNORM", "bc2", base::CompileTimeCRC32("BC2_UNORM").crc(), 4, 8, true, true, ImageFormatClass::UNORM },
        { "BC3_UNORM", "bc3", base::CompileTimeCRC32("BC3_UNORM").crc(), 4, 8, true, true, ImageFormatClass::UNORM },
        { "BC4_UNORM", "bc4", base::CompileTimeCRC32("BC4_UNORM").crc(), 4, 8, true, true, ImageFormatClass::UNORM },
        { "BC5_UNORM", "bc5", base::CompileTimeCRC32("BC5_UNORM").crc(), 4, 8, true, true, ImageFormatClass::UNORM },
        { "BC6_UNSIGNED", "bc6ui", base::CompileTimeCRC32("BC6_UINT").crc(), 2, 8, true, true, ImageFormatClass::FLOAT },
        { "BC6_SIGNED", "bc6i", base::CompileTimeCRC32("BC6_INT").crc(), 2, 8, true, true, ImageFormatClass::FLOAT },
        { "BC7_UNORM", "bc7", base::CompileTimeCRC32("BC7_UNORM").crc(), 4, 8, true, true, ImageFormatClass::UNORM },

        { "SRGB8", "srgb8", base::CompileTimeCRC32("SRGB8").crc(), 3, 24, false, false, ImageFormatClass::SRGB },
        { "SRGBA8", "srgba8", base::CompileTimeCRC32("SRGBA8").crc(), 4, 32, false, false, ImageFormatClass::SRGB },
        { "BC1_SRGB", "bc1_srgb", base::CompileTimeCRC32("BC1_SRGB").crc(), 4, 4, true, true, ImageFormatClass::SRGB },
        { "BC2_SRGB", "bc2_srgb", base::CompileTimeCRC32("BC2_SRGB").crc(), 4, 8, true, true, ImageFormatClass::SRGB },
        { "BC3_SRGB", "bc3_srgb", base::CompileTimeCRC32("BC3_SRGB").crc(), 4, 8, true, true, ImageFormatClass::SRGB },
        { "BC7_SRGB", "bc7_srgb", base::CompileTimeCRC32("BC7_SRGB").crc(), 4, 8, true, true, ImageFormatClass::SRGB },

        { "D24S8", "d24s8", base::CompileTimeCRC32("D24S8").crc(), 2, 32, true, true, ImageFormatClass::DEPTH },
        { "D24FS8", "d24fs8", base::CompileTimeCRC32("D24FS8").crc(), 2, 32, true, true, ImageFormatClass::DEPTH },
        { "D32", "d32", base::CompileTimeCRC32("D32").crc(), 1, 32, true, true, ImageFormatClass::DEPTH },
    };

    const ImageFormatInfo& GetImageFormatInfo(ImageFormat format)
    {
        auto index = (uint32_t)format;
        if (index >= (uint32_t)ImageFormat::MAX)
            return GImageFormatInfos[0];

#ifdef BUILD_DEBUG
        auto name  = base::reflection::GetEnumValueName<ImageFormat>(format);
        auto formatName  = GImageFormatInfos[index].name;
        DEBUG_CHECK(0 == strcmp(name, formatName));
#endif

        return GImageFormatInfos[index];
    }

    bool GetImageFormatByDisplayName(base::StringView<char> name, ImageFormat& outFormat)
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

    bool GetImageFormatByShaderName(base::StringView<char> name, ImageFormat& outFormat)
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

} // rendering
