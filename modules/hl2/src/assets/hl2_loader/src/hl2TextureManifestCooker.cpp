/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
***/

#include "build.h"

#include "base/io/include/ioFileHandle.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "rendering/texture/include/renderingTexture.h"
#include "rendering/texture/include/renderingStaticTexture.h"
#include "base/resource/include/resourceCooker.h"
#include "base/resource/include/resourceTags.h"

namespace hl2
{
    namespace vtf
    {
#pragma pack(push)
#pragma pack(1)
        struct Header
        {
            char            signature[4];		// File signature ("VTF\0"). (or as little-endian integer, 0x00465456)
            unsigned int    version[2];		    // version[0].version[1] (currently 7.2).
            unsigned int	headerSize;		    // Size of the header struct  (16 byte aligned; currently 80 bytes) + size of the resources dictionary (7.3+).
            unsigned short	width;			    // Width of the largest mipmap in pixels. Must be a power of 2.
            unsigned short	height;			    // Height of the largest mipmap in pixels. Must be a power of 2.
            unsigned int	flags;			    // VTF flags.
            unsigned short	frames;			    // Number of frames, if animated (1 for no animation).
            unsigned short	firstFrame;		    // First frame in animation (0 based).
            unsigned char	padding0[4];		// reflectivity padding (16 byte alignment).
            float		    reflectivity[3];	// reflectivity vector.
            unsigned char	padding1[4];		// reflectivity padding (8 byte packing).
            float		    bumpmapScale;		// Bumpmap scale.
            unsigned int	highResImageFormat;	// High resolution image format.
            unsigned char	mipmapCount;		// Number of mipmaps.
            unsigned int	lowResImageFormat;	// Low resolution image format (always DXT1).
            unsigned char	lowResImageWidth;	// Low resolution image width.
            unsigned char	lowResImageHeight;	// Low resolution image height.

            // 7.2+
            unsigned short	depth;			    // Depth of the largest mipmap in pixels.
            // Must be a power of 2. Can be 0 or 1 for a 2D texture (v7.2 only).

            // 7.3+
            unsigned char	padding2[3];		// depth padding (4 byte alignment).
            unsigned int	numResources;		// Number of resources this vtf has
        };

        struct ResourceInfo
        {
            union
            {
                uint32_t Type;
                struct
                {
                    uint8_t ID[3];	//!< Unique resource ID
                    uint8_t Flags;	//!< Resource flags
                };
            };
            uint32_t Data;	//!< Resource data (e.g. for a  CRC) or offset from start of the file
        };

        struct FormatInfo
        {
            const char* name;
            uint32_t bitsPerPixel;
            uint32_t bytesPerPixel;
            short conversionFormat;
            bool bIsCompressed;
            bool bRequiresUnpacking;
        };
#pragma pack()
#pragma pack(pop)

#define MAKE_VTF_RSRC_ID(a, b, c) ((uint32_t)(((uint8_t)a) | ((uint8_t)b << 8) | ((uint8_t)c << 16)))
#define MAKE_VTF_RSRC_IDF(a, b, c, d) ((uint32_t)(((uint8_t)a) | ((uint8_t)b << 8) | ((uint8_t)c << 16) | ((uint8_t)d << 24)))

        enum VTFResourceEntryTypeFlag
        {
            RSRCF_HAS_NO_DATA_CHUNK = 0x02
        };

        enum VTFResourceEntryType
        {
            VTF_LEGACY_RSRC_LOW_RES_IMAGE = MAKE_VTF_RSRC_ID(0x01, 0, 0),
            VTF_LEGACY_RSRC_IMAGE = MAKE_VTF_RSRC_ID(0x30, 0, 0),
            VTF_RSRC_SHEET = MAKE_VTF_RSRC_ID(0x10, 0, 0),
            VTF_RSRC_CRC = MAKE_VTF_RSRC_IDF('C', 'R', 'C', RSRCF_HAS_NO_DATA_CHUNK),
            VTF_RSRC_TEXTURE_LOD_SETTINGS = MAKE_VTF_RSRC_IDF('L', 'O', 'D', RSRCF_HAS_NO_DATA_CHUNK),
            VTF_RSRC_TEXTURE_SETTINGS_EX = MAKE_VTF_RSRC_IDF('T', 'S', 'O', RSRCF_HAS_NO_DATA_CHUNK),
            VTF_RSRC_KEY_VALUE_DATA = MAKE_VTF_RSRC_ID('K', 'V', 'D'),
            VTF_RSRC_MAX_DICTIONARY_ENTRIES = 32
        };

        enum
        {
            IMAGE_FORMAT_NONE = -1,
            IMAGE_FORMAT_RGBA8888 = 0,
            IMAGE_FORMAT_ABGR8888,
            IMAGE_FORMAT_RGB888,
            IMAGE_FORMAT_BGR888,
            IMAGE_FORMAT_RGB565,
            IMAGE_FORMAT_I8,
            IMAGE_FORMAT_IA88,
            IMAGE_FORMAT_P8,
            IMAGE_FORMAT_A8,
            IMAGE_FORMAT_RGB888_BLUESCREEN,
            IMAGE_FORMAT_BGR888_BLUESCREEN,
            IMAGE_FORMAT_ARGB8888,
            IMAGE_FORMAT_BGRA8888,
            IMAGE_FORMAT_DXT1,
            IMAGE_FORMAT_DXT3,
            IMAGE_FORMAT_DXT5,
            IMAGE_FORMAT_BGRX8888,
            IMAGE_FORMAT_BGR565,
            IMAGE_FORMAT_BGRX5551,
            IMAGE_FORMAT_BGRA4444,
            IMAGE_FORMAT_DXT1_ONEBITALPHA,
            IMAGE_FORMAT_BGRA5551,
            IMAGE_FORMAT_UV88,
            IMAGE_FORMAT_UVWQ8888,
            IMAGE_FORMAT_RGBA16161616F,
            IMAGE_FORMAT_RGBA16161616,
            IMAGE_FORMAT_UVLX8888,
            IMAGE_FORMAT_R32F,
            IMAGE_FORMAT_RGB323232F,
            IMAGE_FORMAT_RGBA32323232F,
        };

        enum
        {
            // Flags from the *.txt config file
                TEXTUREFLAGS_POINTSAMPLE = 0x00000001,
            TEXTUREFLAGS_TRILINEAR = 0x00000002,
            TEXTUREFLAGS_CLAMPS = 0x00000004,
            TEXTUREFLAGS_CLAMPT = 0x00000008,
            TEXTUREFLAGS_ANISOTROPIC = 0x00000010,
            TEXTUREFLAGS_HINT_DXT5 = 0x00000020,
            TEXTUREFLAGS_PWL_CORRECTED = 0x00000040,
            TEXTUREFLAGS_NORMAL = 0x00000080,
            TEXTUREFLAGS_NOMIP = 0x00000100,
            TEXTUREFLAGS_NOLOD = 0x00000200,
            TEXTUREFLAGS_ALL_MIPS = 0x00000400,
            TEXTUREFLAGS_PROCEDURAL = 0x00000800,

            // These are automatically generated by vtex from the texture data.
                TEXTUREFLAGS_ONEBITALPHA = 0x00001000,
            TEXTUREFLAGS_EIGHTBITALPHA = 0x00002000,

            // Newer flags from the *.txt config file
                TEXTUREFLAGS_ENVMAP = 0x00004000,
            TEXTUREFLAGS_RENDERTARGET = 0x00008000,
            TEXTUREFLAGS_DEPTHRENDERTARGET = 0x00010000,
            TEXTUREFLAGS_NODEBUGOVERRIDE = 0x00020000,
            TEXTUREFLAGS_SINGLECOPY	= 0x00040000,
            TEXTUREFLAGS_PRE_SRGB = 0x00080000,

            TEXTUREFLAGS_UNUSED_00100000 = 0x00100000,
            TEXTUREFLAGS_UNUSED_00200000 = 0x00200000,
            TEXTUREFLAGS_UNUSED_00400000 = 0x00400000,

            TEXTUREFLAGS_NODEPTHBUFFER = 0x00800000,

            TEXTUREFLAGS_UNUSED_01000000 = 0x01000000,

            TEXTUREFLAGS_CLAMPU = 0x02000000,
            TEXTUREFLAGS_VERTEXTEXTURE = 0x04000000,
            TEXTUREFLAGS_SSBUMP = 0x08000000,

            TEXTUREFLAGS_UNUSED_10000000 = 0x10000000,

            TEXTUREFLAGS_BORDER = 0x20000000,

            TEXTUREFLAGS_UNUSED_40000000 = 0x40000000,
            TEXTUREFLAGS_UNUSED_80000000 = 0x80000000,
        };

        static FormatInfo ImageFormatInfos[] =
            {
            //   0 -> 24
                { "RGBA8888",			 32,  4, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_RGBA8888,
                { "ABGR8888",			 32,  4, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_ABGR8888,
                { "RGB888",				 24,  3, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_RGB888,
                { "BGR888",				 24,  3, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_BGR888,
                { "RGB565",				 16,  2, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_RGB565,
                { "I8",					  8,  1, IMAGE_FORMAT_A8, false,  true },		// IMAGE_FORMAT_I8,
                { "IA88",				 16,  2, IMAGE_FORMAT_IA88, false,  true },		// IMAGE_FORMAT_IA88
                { "P8",					  8,  1, IMAGE_FORMAT_A8, false, false },		// IMAGE_FORMAT_P8
                { "A8",					  8,  1, IMAGE_FORMAT_A8, false,  true },		// IMAGE_FORMAT_A8
                { "RGB888 Bluescreen",	 24,  3, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_RGB888_BLUESCREEN
                { "BGR888 Bluescreen",	 24,  3, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_BGR888_BLUESCREEN
                { "ARGB8888",			 32,  4, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_ARGB8888
                { "BGRA8888",			 32,  4, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_BGRA8888
                { "DXT1",				  4,  0, IMAGE_FORMAT_DXT1, true,  true },		// IMAGE_FORMAT_DXT1
                { "DXT3",				  8,  0, IMAGE_FORMAT_DXT3,  true,  true },		// IMAGE_FORMAT_DXT3
                { "DXT5",				  8,  0, IMAGE_FORMAT_DXT5,  true,  true },		// IMAGE_FORMAT_DXT5
                { "BGRX8888",			 32,  4, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_BGRX8888
                { "BGR565",				 16,  2, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_BGR565
                { "BGRX5551",			 16,  2, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_BGRX5551
                { "BGRA4444",			 16,  2, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_BGRA4444
                { "DXT1 One Bit Alpha",	  4,  0, IMAGE_FORMAT_DXT1_ONEBITALPHA, true,  true },		// IMAGE_FORMAT_DXT1_ONEBITALPHA
                { "BGRA5551",			 16,  2, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_BGRA5551
                { "UV88",				 16,  2, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_UV88
                { "UVWQ8888",			 32,  4, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_UVWQ8899
                { "RGBA16161616F",	     64,  8, IMAGE_FORMAT_RGBA16161616F, false,  true },		// IMAGE_FORMAT_RGBA16161616F
                { "RGBA16161616",	     64,  8, IMAGE_FORMAT_RGBA16161616, false,  true },		// IMAGE_FORMAT_RGBA16161616
                { "UVLX8888",			 32,  4, IMAGE_FORMAT_RGBA8888, false,  true },		// IMAGE_FORMAT_UVLX8888
                { "R32F",				 32,  4, IMAGE_FORMAT_R32F, false,  true },		// IMAGE_FORMAT_R32F
                { "RGB323232F",			 96, 12, IMAGE_FORMAT_RGBA32323232F, false,  true },		// IMAGE_FORMAT_RGB323232F
                { "RGBA32323232F",		128, 16, IMAGE_FORMAT_RGBA32323232F, false,  true },		// IMAGE_FORMAT_RGBA32323232F
                { "nVidia DST16",		 16,  2, IMAGE_FORMAT_NONE, false,  true },		// IMAGE_FORMAT_NV_DST16
                { "nVidia DST24",		 24,  3, IMAGE_FORMAT_NONE, false,  true },		// IMAGE_FORMAT_NV_DST24
                { "nVidia INTZ",		 32,  4, IMAGE_FORMAT_NONE, false,  true },		// IMAGE_FORMAT_NV_INTZ
                { "nVidia RAWZ",		 32,  4, IMAGE_FORMAT_NONE, false,  true },		// IMAGE_FORMAT_NV_RAWZ
                { "ATI DST16",			 16,  2, IMAGE_FORMAT_NONE, false,  true },		// IMAGE_FORMAT_ATI_DST16
                { "ATI DST24",			 24,  3, IMAGE_FORMAT_NONE, false,  true },		// IMAGE_FORMAT_ATI_DST24
                { "nVidia NULL",		 32,  4, IMAGE_FORMAT_NONE, false,  true },		// IMAGE_FORMAT_NV_NULL
                { "ATI1N",				  4,  0, IMAGE_FORMAT_NONE,  true,  true },		// IMAGE_FORMAT_ATI1N
                { "ATI2N",				  8,  0, IMAGE_FORMAT_NONE,  true,  true }/*,		// IMAGE_FORMAT_ATI2N
	{ "Xbox360 DST16",		 16,  0,  0,  0,  0,  0, false,  true },		// IMAGE_FORMAT_X360_DST16
	{ "Xbox360 DST24",		 24,  0,  0,  0,  0,  0, false,  true },		// IMAGE_FORMAT_X360_DST24
	{ "Xbox360 DST24F",		 24,  0,  0,  0,  0,  0, false , true },		// IMAGE_FORMAT_X360_DST24F
	{ "Linear BGRX8888",	 32,  4,  8,  8,  8,  0, false,  true },		// IMAGE_FORMAT_LINEAR_BGRX8888
	{ "Linear RGBA8888",     32,  4,  8,  8,  8,  8, false,  true },		// IMAGE_FORMAT_LINEAR_RGBA8888
	{ "Linear ABGR8888",	 32,  4,  8,  8,  8,  8, false,  true },		// IMAGE_FORMAT_LINEAR_ABGR8888
	{ "Linear ARGB8888",	 32,  4,  8,  8,  8,  8, false,  true },		// IMAGE_FORMAT_LINEAR_ARGB8888
	{ "Linear BGRA8888",	 32,  4,  8,  8,  8,  8, false,  true },		// IMAGE_FORMAT_LINEAR_BGRA8888
	{ "Linear RGB888",		 24,  3,  8,  8,  8,  0, false,  true },		// IMAGE_FORMAT_LINEAR_RGB888
	{ "Linear BGR888",		 24,  3,  8,  8,  8,  0, false,  true },		// IMAGE_FORMAT_LINEAR_BGR888
	{ "Linear BGRX5551",	 16,  2,  5,  5,  5,  0, false,  true },		// IMAGE_FORMAT_LINEAR_BGRX5551
	{ "Linear I8",			  8,  1,  0,  0,  0,  0, false,  true },		// IMAGE_FORMAT_LINEAR_I8
	{ "Linear RGBA16161616", 64,  8, 16, 16, 16, 16, false,  true },		// IMAGE_FORMAT_LINEAR_RGBA16161616
	{ "LE BGRX8888",         32,  4,  8,  8,  8,  0, false,  true },		// IMAGE_FORMAT_LE_BGRX8888
	{ "LE BGRA8888",		 32,  4,  8,  8,  8,  8, false,  true }*/		// IMAGE_FORMAT_LE_BGRA8888
            };

        static rendering::ImageFormat GetCompiledFormat(short ImageFormat)
        {
            switch (ImageFormat)
            {
                case IMAGE_FORMAT_RGBA8888:
                    return rendering::ImageFormat::RGBA8_UNORM;
                case IMAGE_FORMAT_A8:
                case IMAGE_FORMAT_I8:
                    return rendering::ImageFormat::R8_UNORM;
                case IMAGE_FORMAT_IA88:
                    return rendering::ImageFormat::RG8_UNORM;
                case IMAGE_FORMAT_DXT1:
                case IMAGE_FORMAT_DXT1_ONEBITALPHA:
                    return rendering::ImageFormat::BC1_UNORM;
                case IMAGE_FORMAT_DXT3:
                    return rendering::ImageFormat::BC2_UNORM;
                case IMAGE_FORMAT_DXT5:
                    return rendering::ImageFormat::BC3_UNORM;
                case IMAGE_FORMAT_RGBA16161616F:
                    return rendering::ImageFormat::RGBA16F;
                case IMAGE_FORMAT_RGBA16161616:
                    return rendering::ImageFormat::RGBA16_UNORM;
                case IMAGE_FORMAT_RGBA32323232F:
                    return rendering::ImageFormat::RGBA32F;
                case IMAGE_FORMAT_R32F:
                    return rendering::ImageFormat::R32F;
            }

            ASSERT(!"Invalid conversion format");
            return rendering::ImageFormat::UNKNOWN;
        }

        static const FormatInfo& GetImageFormat(uint32_t ImageFormat)
        {
            ASSERT_EX(ImageFormat < ARRAY_COUNT(ImageFormatInfos), "Invalid format");
            return ImageFormatInfos[ImageFormat];
        }

        static uint32_t ComputeImageSize(uint32_t uiWidth, uint32_t uiHeight, uint32_t uiDepth, uint32_t ImageFormat)
        {
            switch (ImageFormat)
            {
                case IMAGE_FORMAT_DXT1:
                case IMAGE_FORMAT_DXT1_ONEBITALPHA:
                    if(uiWidth < 4 && uiWidth > 0)
                        uiWidth = 4;

                    if(uiHeight < 4 && uiHeight > 0)
                        uiHeight = 4;

                    return ((uiWidth + 3) / 4) * ((uiHeight + 3) / 4) * 8 * uiDepth;
                case IMAGE_FORMAT_DXT3:
                case IMAGE_FORMAT_DXT5:
                    if(uiWidth < 4 && uiWidth > 0)
                        uiWidth = 4;

                    if(uiHeight < 4 && uiHeight > 0)
                        uiHeight = 4;

                    return ((uiWidth + 3) / 4) * ((uiHeight + 3) / 4) * 16 * uiDepth;
                default:
                    return uiWidth * uiHeight * uiDepth * GetImageFormat(ImageFormat).bytesPerPixel;
            }
        }

        static uint32_t ComputeImageSize(uint32_t uiWidth, uint32_t uiHeight, uint32_t uiDepth, uint32_t uiMipmaps, uint32_t ImageFormat)
        {
            uint32_t uiImageSize = 0;

            for (uint32_t i = 0; i < uiMipmaps; i++)
            {
                uiImageSize += ComputeImageSize(uiWidth, uiHeight, uiDepth, ImageFormat);

                uiWidth >>= 1;
                uiHeight >>= 1;
                uiDepth >>= 1;

                if(uiWidth < 1)
                    uiWidth = 1;

                if(uiHeight < 1)
                    uiHeight = 1;

                if(uiDepth < 1)
                    uiDepth = 1;
            }

            return uiImageSize;
        }

        static void ComputeMipmapDimensions(uint32_t uiWidth, uint32_t uiHeight, uint32_t uiDepth, uint32_t uiMipmapLevel, uint32_t &uiMipmapWidth, uint32_t &uiMipmapHeight, uint32_t &uiMipmapDepth)
        {
            // work out the width/height by taking the orignal dimension
            // and bit shifting them down uiMipmapLevel times
            uiMipmapWidth = uiWidth >> uiMipmapLevel;
            uiMipmapHeight = uiHeight >> uiMipmapLevel;
            uiMipmapDepth = uiDepth >> uiMipmapLevel;

            // stop the dimension being less than 1 x 1
            if(uiMipmapWidth < 1)
                uiMipmapWidth = 1;

            if(uiMipmapHeight < 1)
                uiMipmapHeight = 1;

            if(uiMipmapDepth < 1)
                uiMipmapDepth = 1;
        }

        static uint32_t ComputeMipmapSize(uint32_t uiWidth, uint32_t uiHeight, uint32_t uiDepth, uint32_t uiMipmapLevel, uint32_t ImageFormat)
        {
            // figure out the width/height of this MIP level
            uint32_t uiMipmapWidth, uiMipmapHeight, uiMipmapDepth;
            ComputeMipmapDimensions(uiWidth, uiHeight, uiDepth, uiMipmapLevel, uiMipmapWidth, uiMipmapHeight, uiMipmapDepth);

            // return the memory requirements
            return ComputeImageSize(uiMipmapWidth, uiMipmapHeight, uiMipmapDepth, ImageFormat);
        }

        static uint32_t GetFaceCount(const vtf::Header& header)
        {
            if (header.flags & TEXTUREFLAGS_ENVMAP)
            {
                if (header.version[1] < 5)
                    return 7;
                else
                    return 6;
            }

            return 1;
        }

        static uint32_t ComputeDataOffset(const vtf::Header& header, uint32_t uiFrame, uint32_t uiFace, uint32_t uiSlice, uint32_t uiMipLevel, uint32_t ImageFormat)
        {
            uint32_t uiOffset = 0;

            auto uiFrameCount = header.frames;
            auto uiFaceCount = GetFaceCount(header);
            auto uiSliceCount = header.depth;
            auto uiMipCount = header.mipmapCount;

            if (uiFrame >= uiFrameCount)
                uiFrame = uiFrameCount - 1;

            if (uiFace >= uiFaceCount)
                uiFace = uiFaceCount - 1;

            if (uiSlice >= uiSliceCount)
                uiSlice = uiSliceCount - 1;

            if (uiMipLevel >= uiMipCount)
                uiMipLevel = uiMipCount - 1;

            // Transverse past all frames and faces of each mipmap (up to the requested one).
            for (int i = (int)uiMipCount - 1; i > (uint32_t)uiMipLevel; i--)
            {
                uiOffset += ComputeMipmapSize(header.width, header.height, header.depth, i, ImageFormat) * uiFrameCount * uiFaceCount;
            }

            auto uiTemp1 = ComputeMipmapSize(header.width, header.height, header.depth, uiMipLevel, ImageFormat);
            auto uiTemp2 = ComputeMipmapSize(header.width, header.height, 1, uiMipLevel, ImageFormat);

            // Transverse past requested frames and faces of requested mipmap.
            uiOffset += uiTemp1 * uiFrame * uiFaceCount * uiSliceCount;
            uiOffset += uiTemp1 * uiFace * uiSliceCount;
            uiOffset += uiTemp2 * uiSlice;

            return uiOffset;
        }

        static uint32_t GetTargetImageSize(uint32_t width, uint32_t height, uint32_t depth, uint32_t format)
        {
            auto& targetFormatInfo = GetImageFormat(format);
            return width*height*depth * targetFormatInfo.bytesPerPixel;
        }

        struct UnpackedPixel
        {
            uint32_t x,y,z,w;

            static void UnpackRGBA8888(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[0];
                p.y = s[1];
                p.z = s[2];
                p.w = s[3];
                s += 4;
            }

            static void UnpackABGR8888(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[3];
                p.y = s[2];
                p.z = s[1];
                p.w = s[0];
                s += 4;
            }

            static void UnpackARGB8888(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[1];
                p.y = s[2];
                p.z = s[3];
                p.w = s[0];
                s += 4;
            }

            static void UnpackBGRA8888(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[2];
                p.y = s[1];
                p.z = s[0];
                p.w = s[3];
                s += 4;
            }

            static void UnpackRGB888(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[0];
                p.y = s[1];
                p.z = s[2];
                p.w = 255;
                s += 3;
            }

            static void UnpackBGR888(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[2];
                p.y = s[1];
                p.z = s[0];
                p.w = 255;
                s += 3;
            }

            static void UnpackBGRX8888(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[2];
                p.y = s[1];
                p.z = s[0];
                p.w = 255;
                s += 4;
            }

            static void UnpackRGB565(UnpackedPixel& p, const uint8_t*& s)
            {
                uint16_t data = *(const uint16_t*)s;
                p.x = (data & 0x001F) << 3;
                p.y = (data & 0x07E0) >> 3;
                p.z = (data & 0xF800) >> 8;
                p.w = 255;
                s += 2;
            }

            static void UnpackBGR565(UnpackedPixel& p, const uint8_t*& s)
            {
                uint16_t data = *(const uint16_t*)s;
                p.z = (data & 0x001F) << 3;
                p.y = (data & 0x07E0) >> 3;
                p.x = (data & 0xF800) >> 8;
                p.w = 255;
                s += 2;
            }

            static void UnpackI8(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[0];
                p.y = 255;
                p.z = 255;
                p.w = 255;
                s += 1;
            }

            static void UnpackA8(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[0];
                p.y = 255;
                p.z = 255;
                p.w = 255;
                s += 1;
            }

            static void UnpackP8(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[0];
                p.y = 255;
                p.z = 255;
                p.w = 255;
                s += 1;
            }

            static void UnpackIA88(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[0];
                p.y = s[1];
                p.z = 255;
                p.w = 255;
                s += 2;
            }

            static void UnpackRGB888_BLUESCREEN(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[0];
                p.y = s[1];
                p.z = s[2];
                p.w = 255;
                s += 3;
            }

            static void UnpackBGR888_BLUESCREEN(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[2];
                p.y = s[1];
                p.z = s[0];
                p.w = 255;
                s += 3;
            }

            static void UnpackBGRX5551(UnpackedPixel& p, const uint8_t*& s)
            {
                uint16_t data = *(const uint16_t*)s;
                p.z = (data & 0x001F) << 3;
                p.y = (data & 0x03E0) >> 2;
                p.x = (data & 0x7C00) >> 3;
                p.w = (data & 0x8000) ? 255 : 0;
                s += 2;
            }

            static void UnpackBGRA5551(UnpackedPixel& p, const uint8_t*& s)
            {
                uint16_t data = *(const uint16_t*)s;
                p.z = (data & 0x001F) << 3;
                p.y = (data & 0x03E0) >> 2;
                p.x = (data & 0x7C00) >> 3;
                p.w = (data & 0x8000) ? 255 : 0;
                s += 2;
            }

            static void UnpackBGRA4444(UnpackedPixel& p, const uint8_t*& s)
            {
                uint16_t data = *(const uint16_t*)s;
                p.z = (data >> 0) & 0xF;
                p.y = (data >> 4) & 0xF;
                p.x = (data >> 8) & 0xF;
                p.w = (data >> 12) & 0xF;
                s += 2;
            }

            static void UnpackUV88(UnpackedPixel& p, const uint8_t*& s)
            {
                auto dx = std::clamp<float>((float)(char)s[0] / 127.0f, -1.0f, 1.0f);
                auto dy = std::clamp<float>((float)(char)s[1] / 127.0f, -1.0f, 1.0f);
                auto dz = sqrt(1.0f - dx*dx + dy*dy);

                p.x = (uint8_t)std::clamp<float>(127.0f + dx * 127.0f, 0.0f, 255.0f);
                p.y = (uint8_t)std::clamp<float>(127.0f + dy * 127.0f, 0.0f, 255.0f);
                p.z = (uint8_t)std::clamp<float>(127.0f + dz * 127.0f, 0.0f, 255.0f);
                p.w = 255;
                s += 2;
            }

            static void UnpackUVWQ8888(UnpackedPixel& p, const uint8_t*& s)
            {
                p.x = s[0];
                p.y = s[1];
                p.z = s[2];
                p.w = s[3];
                s += 2;
            }

            static void UnpackRGBA16161616F(UnpackedPixel& p, const uint8_t*& s)
            {
                auto data  = (uint16_t*)s;
                p.x = data[0];
                p.y = data[1];
                p.z = data[2];
                p.w = data[3];
                s += 8;
            }

            static void UnpackRGBA16161616(UnpackedPixel& p, const uint8_t*& s)
            {
                auto data  = (uint16_t*)s;
                p.x = data[0];
                p.y = data[1];
                p.z = data[2];
                p.w = data[3];
                s += 8;
            }

            static void UnpackR32F(UnpackedPixel& p, const uint8_t*& s)
            {
                auto data  = (uint32_t*)s;
                p.x = data[0];
                p.y = 0;
                p.z = 0;
                p.w = 0;
                s += 4;
            }

            static void UnpackRGB323232F(UnpackedPixel& p, const uint8_t*& s)
            {
                auto data  = (uint32_t*)s;
                p.x = data[0];
                p.y = data[1];
                p.z = data[2];
                p.w = 0;
                s += 12;
            }

            static void UnpackRGBA32323232F(UnpackedPixel& p, const uint8_t*& s)
            {
                auto data  = (uint32_t*)s;
                p.x = data[0];
                p.y = data[1];
                p.z = data[2];
                p.w = data[3];
                s += 16;
            }

            static void PackRGBA8888(const UnpackedPixel& p, uint8_t*& w)
            {
                w[0] = (uint8_t)p.x;
                w[1] = (uint8_t)p.y;
                w[2] = (uint8_t)p.z;
                w[3] = (uint8_t)p.w;
                w += 4;
            }

            static void PackA8(const UnpackedPixel& p, uint8_t*& w)
            {
                w[0] = (uint8_t)p.x;
                w += 1;
            }

            static void PackI8(const UnpackedPixel& p, uint8_t*& w)
            {
                w[0] = (uint8_t)p.x;
                w += 1;
            }

            static void PackIA88(const UnpackedPixel& p, uint8_t*& w)
            {
                w[0] = (uint8_t)p.x;
                w[1] = (uint8_t)p.y;
                w += 2;
            }

            static void PackRGBA16161616F(const UnpackedPixel& p, uint8_t*& w)
            {
                auto data  = (uint16_t*)w;
                w[0] = (uint16_t)p.x;
                w[1] = (uint16_t)p.y;
                w[2] = (uint16_t)p.z;
                w[3] = (uint16_t)p.w;
                w += 8;
            }

            static void PackRGBA16161616(const UnpackedPixel& p, uint8_t*& w)
            {
                auto data  = (uint16_t*)w;
                w[0] = (uint16_t)p.x;
                w[1] = (uint16_t)p.y;
                w[2] = (uint16_t)p.z;
                w[3] = (uint16_t)p.w;
                w += 8;
            }

            static void PackRGBA32323232F(const UnpackedPixel& p, uint8_t*& w)
            {
                auto data  = (uint32_t*)w;
                w[0] = p.x;
                w[1] = p.y;
                w[2] = p.z;
                w[3] = p.w;
                w += 16;
            }

            static void PackR32F(const UnpackedPixel& p, uint8_t*& w)
            {
                auto data  = (uint32_t*)w;
                w[0] = p.x;
                w += 4;
            }
        };

        template< typename Func >
        INLINE void UnpackDataInner(const uint8_t* srcData, uint32_t width, uint32_t height, uint32_t depth, UnpackedPixel* targetData, const Func& func)
        {
            uint32_t count = width*height*depth;
            for (uint32_t i=0; i<count; ++i, ++targetData)
                func(*targetData, srcData);
        }

        static void UnpackData(const uint8_t* srcData, uint32_t width, uint32_t height, uint32_t depth, UnpackedPixel* targetData, uint32_t imageFormat)
        {
            #define TEST(x) case IMAGE_FORMAT_##x: UnpackDataInner(srcData, width, height, depth, targetData, &UnpackedPixel::Unpack##x); return;
            switch (imageFormat)
            {
                TEST(RGBA8888);
                TEST(ABGR8888);
                TEST(RGB888);
                TEST(BGR888);
                TEST(RGB565);
                TEST(I8);
                TEST(IA88);
                TEST(P8);
                TEST(A8);
                TEST(RGB888_BLUESCREEN);
                TEST(BGR888_BLUESCREEN);
                TEST(ARGB8888);
                TEST(BGRA8888);
                TEST(BGRX8888);
                TEST(BGR565);
                TEST(BGRX5551);
                TEST(BGRA4444);
                TEST(BGRA5551);
                TEST(UV88);
                TEST(UVWQ8888);
                TEST(RGBA16161616F);
                TEST(RGBA16161616);
                TEST(R32F);
                TEST(RGB323232F);
                TEST(RGBA32323232F);
            }
            #undef TEST

            ASSERT(!"Unsupported format");
        }

        template< typename Func >
        INLINE void PackDataInner(const UnpackedPixel* srcData, uint32_t width, uint32_t height, uint32_t depth, uint8_t* targetData, const Func& func)
        {
            uint32_t count = width*height*depth;
            for (uint32_t i=0; i<count; ++i, ++srcData)
                func(*srcData, targetData);
        }

        static void PackData(const UnpackedPixel* srcData, uint32_t width, uint32_t height, uint32_t depth, uint8_t* targetData, uint32_t imageFormat)
        {
#define TEST(x) case IMAGE_FORMAT_##x: PackDataInner(srcData, width, height, depth, targetData, &UnpackedPixel::Pack##x); return;
            switch (imageFormat)
            {
                TEST(RGBA8888);
                TEST(I8);
                TEST(IA88);
                TEST(A8);
                TEST(RGBA16161616F);
                TEST(RGBA16161616);
                TEST(R32F);
                TEST(RGBA32323232F);
            }
#undef TEST

            ASSERT(!"Unsupported format");
        }

        static base::Buffer CreateImageBuffer(uint32_t width, uint32_t height, uint32_t depth, uint32_t sourceFormat, uint8_t* sourceData, uint32_t sourceSize)
        {
            // data with the same conversion format can be used directly
            auto& sourceFormatInfo = GetImageFormat(sourceFormat);
            if (sourceFormatInfo.bIsCompressed || sourceFormatInfo.conversionFormat == sourceFormat)
                return base::Buffer::Create(POOL_TEMP, sourceSize, 1, sourceData);

            // allocate buffer
            base::Array<UnpackedPixel> tempPixels;
            tempPixels.resize(width*height*depth);

            // unpack data
            UnpackData(sourceData, width, height, depth, tempPixels.typedData(), sourceFormat);

            // allocate output buffer
            auto outputSize = GetTargetImageSize(width, height, depth, sourceFormatInfo.conversionFormat);
            auto outputBuffer = base::Buffer::Create(POOL_TEMP, outputSize, 1);

            // pack data
            PackData(tempPixels.typedData(), width, height, depth, (uint8_t*)outputBuffer.data(), sourceFormatInfo.conversionFormat);
            return outputBuffer;
        }

    } // vtf

    /// manifest for cooking VTF texture
    class TextureCookerManifestVTF : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(TextureCookerManifestVTF, base::res::IResourceCooker);

    public:
        TextureCookerManifestVTF()
        {}

        virtual base::res::ResourcePtr cook(base::res::IResourceCookerInterface& cooker) const override final
        {
            // we should cook a texture
            const auto& sourceFilePath = cooker.queryResourcePath();

            // load into buffer
            auto data = cooker.loadToBuffer(sourceFilePath);
            if (!data)
                return nullptr;

            // load the header
            if (data.size() < sizeof(vtf::Header))
            {
                TRACE_ERROR("Texture has no header");
                return nullptr;
            }

            // get the header
            vtf::Header header;
            memcpy(&header, data.data(), sizeof(header));
            if (header.signature[0] != 'V' || header.signature[1] != 'T' || header.signature[2] != 'F' || header.signature[3] != 0)
            {
                TRACE_ERROR("Resource '{}' is not a VTF texture", sourceFilePath);
                return nullptr;
            }

            // check version
            if (header.version[0] != 7 || (header.version[1] < 1 || header.version[1] > 5))
            {
                TRACE_ERROR("Resource '{}' is in unsupported VTF version {}.{}", sourceFilePath, header.version[0], header.version[1]);
                return nullptr;
            }

            // no resources
            if (header.version[1] <= 2)
                header.depth = 1;
            if (header.version[1] < 3)
                header.numResources = 0;

            // print info
            TRACE_INFO("Texture file version: {}.{} (header {})", header.version[0], header.version[1], header.headerSize);
            TRACE_INFO("Texture size: {}x{}x{}, {} frames, {} mips, {} resources", header.width, header.height, header.depth, header.frames, header.mipmapCount, header.numResources);
            TRACE_INFO("Texture format: {}", vtf::GetImageFormat(header.highResImageFormat).name);

            // get target format after conversion
            auto targetFormat = vtf::GetImageFormat(header.highResImageFormat).conversionFormat;
            TRACE_INFO("Texture converted: {}", vtf::GetImageFormat(targetFormat).name);

            // flags
            if (header.flags & vtf::TEXTUREFLAGS_EIGHTBITALPHA)
            {
                TRACE_INFO("Texture flag: EightBitAlpha");
            }
            else if (header.flags & vtf::TEXTUREFLAGS_ONEBITALPHA)
            {
                TRACE_INFO("Texture flag: OneBitAlpha");
            }
            else
            {
                TRACE_INFO("Texture flag: NoAlpha");
            }

            // prepare mip data
            rendering::TextureInfo texInfo;
            texInfo.format = vtf::GetCompiledFormat(targetFormat);
            texInfo.type = rendering::ImageViewType::View2D;
            texInfo.width = header.width;
            texInfo.height = header.height;
            texInfo.depth = 1;
            texInfo.compressed = rendering::GetImageFormatInfo(texInfo.format).compressed;
            texInfo.colorSpace = (rendering::GetImageFormatInfo(texInfo.format).formatClass == rendering::ImageFormatClass::SRGB) ? base::image::ColorSpace::SRGB : base::image::ColorSpace::Linear;

            // get the offset to data
            uint32_t offsetToImageData = 0;
            if (header.numResources)
            {
                uint32_t extraDataSize = 0;
                auto resources  = (const vtf::ResourceInfo*)base::OffsetPtr(data.data(), sizeof(vtf::Header) + 8);
                for (uint32_t i=0; i<header.numResources; i++)
                {
                    TRACE_INFO("Texture resource[{}]: {}{}{} (flags: {}): {} ({})", i, Hex(resources[i].ID[0]), Hex(resources[i].ID[1]), Hex(resources[i].ID[2]), Hex(resources[i].Flags), resources[i].Data, Hex(resources[i].Data));

                    switch (resources[i].Type)
                    {
                        case vtf::VTF_LEGACY_RSRC_LOW_RES_IMAGE:
                        {
                            TRACE_INFO("Thumbnail size: {}x{}, {}", header.lowResImageWidth, header.lowResImageHeight, vtf::GetImageFormat(header.lowResImageFormat).name);
                            auto thumbnailDataSize = vtf::ComputeImageSize(header.lowResImageWidth, header.lowResImageHeight, 1, header.lowResImageFormat);
                            extraDataSize += thumbnailDataSize;
                            break;
                        }

                        case vtf::VTF_LEGACY_RSRC_IMAGE:
                        {
                            offsetToImageData = resources[i].Data;
                            break;
                        }

                        default:
                        {
                            if (0 == (resources[i].Flags & vtf::RSRCF_HAS_NO_DATA_CHUNK))
                            {
                                auto resourceDataSize = *(const uint32_t*)base::OffsetPtr(data.data(), resources[i].Data);
                                extraDataSize += resourceDataSize + sizeof(uint32_t);
                                TRACE_INFO("Extra file data of size {}", extraDataSize);
                            }
                            break;
                        }
                    }
                }

                // no direct offset to image data specified, tried to estimate
                if (0 == offsetToImageData)
                {
                    offsetToImageData = sizeof(vtf::Header) + extraDataSize;
                }
            }
            else
            {
                // skip over the thumbnail
                if (header.lowResImageFormat != vtf::IMAGE_FORMAT_NONE)
                {
                    TRACE_INFO("Thumbnail size: {}x{}, {}", header.lowResImageWidth, header.lowResImageHeight, vtf::GetImageFormat(header.lowResImageFormat).name);
                    auto thumbnailDataSize = vtf::ComputeImageSize(header.lowResImageWidth, header.lowResImageHeight, 1, header.lowResImageFormat);
                    offsetToImageData = header.headerSize + thumbnailDataSize;
                }
                else
                {
                    offsetToImageData = header.headerSize;
                }
            }

            // cubemap ?
            uint32_t firstFace = 0;
            uint32_t numFaces = 1;
            uint32_t numSlices = 1;
            auto textureType = rendering::ImageViewType::View2D;
            if (header.flags & vtf::TEXTUREFLAGS_ENVMAP)
            {
                textureType = rendering::ImageViewType::ViewCube;
                firstFace = 0;
                numFaces = 6;
            }

            // slice count
            texInfo.slices = std::max<uint32_t>(1, numFaces * numSlices);
            texInfo.mips = header.mipmapCount;

            // add slices
            base::InplaceArray<rendering::StaticTextureMip, 64> mips;
            base::InplaceArray<base::Buffer, 64> mipData;
            uint32_t offsetInBigBuffer = 0;
            const auto pixelBPP = rendering::GetImageFormatInfo(texInfo.format).bitsPerPixel;
            for (uint32_t k=0; k<numFaces; ++k)
            {
                for (uint32_t j = 0; j < numSlices; ++j)
                {
                    for (uint32_t i = 0; i < header.mipmapCount; ++i)
                    {
                        if (cooker.checkCancelation())
                            return nullptr;

                        auto sourceOffset = vtf::ComputeDataOffset(header, 0, firstFace+k, j, i, header.highResImageFormat);
                        auto sourceSize = vtf::ComputeMipmapSize(header.width, header.height, header.depth, i, header.highResImageFormat);
                        auto sourceData = (const uint8_t*)data.data() + offsetToImageData + sourceOffset;

                        // create raw mipmap
                        auto &rawMip = mips.emplaceBack();
                        rawMip.width = std::max<uint16_t>(1, header.width >> i);
                        rawMip.height = std::max<uint16_t>(1, header.height >> i);
                        rawMip.depth = std::max<uint16_t>(1, header.depth >> i);
                        rawMip.dataOffset = base::Align<uint32_t>(offsetInBigBuffer, 16);
                        rawMip.streamed = false;
                        rawMip.compressed = texInfo.compressed;
                        rawMip.rowPitch = (rawMip.width * pixelBPP) / 8;
                        rawMip.slicePitch = rawMip.rowPitch * rawMip.height;

                        // unpack data
                        auto data = vtf::CreateImageBuffer(rawMip.width, rawMip.height, rawMip.depth, header.highResImageFormat, (uint8_t*)sourceData, sourceSize);
                        if (!data)
                        {
                            TRACE_ERROR("Unable to unmap mip {}, slice {}, face {} ({}x{}x{})", i, j, k, rawMip.width, rawMip.height, rawMip.depth);
                            return nullptr;
                        }

                        mipData.pushBack(data);
                        rawMip.dataSize = data.size();
                        offsetInBigBuffer = rawMip.dataOffset + rawMip.dataSize;
                    }
                }
            }

            // create final buffer
            auto finalBuffer = base::Buffer::Create(POOL_IMAGE, offsetInBigBuffer, 16);
            if (!finalBuffer)
            {
                TRACE_ERROR("Not enough memory to allocate final image buffer ({})", MemSize(offsetToImageData));
                return nullptr;
            }

            for (uint32_t i = 0; i < mips.size(); ++i)
            {
                auto* targetPtr = base::OffsetPtr(finalBuffer.data(), mips[i].dataOffset);
                memcpy(targetPtr, mipData[i].data(), mipData[i].size());
            }

            base::res::AsyncBuffer streamingData;
            return base::RefNew<rendering::StaticTexture>(std::move(finalBuffer), std::move(streamingData), std::move(mips), texInfo);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(TextureCookerManifestVTF);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<rendering::StaticTexture>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtension("vtf");
    RTTI_END_TYPE();

} // hl2