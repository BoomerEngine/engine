/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: utils #]
***/

#include "build.h"

namespace rendering
{
    namespace gl4
    {

        ///---

        base::ConfigProperty<bool> cvSupportSRGBTexture("Rendering.GL4", "EnableSRGBTextures", true);

        ///---

        GLenum TranslateTextureType(ImageViewType viewType, bool isMultiSampled)
        {
            switch (viewType)
            {
            case ImageViewType::View1D:
                ASSERT(!isMultiSampled);
                return GL_TEXTURE_1D;

            case ImageViewType::View2D:
                return isMultiSampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

            case ImageViewType::View3D:
                ASSERT(!isMultiSampled);
                return GL_TEXTURE_3D;

            case ImageViewType::ViewCube:
                ASSERT(!isMultiSampled);
                return GL_TEXTURE_CUBE_MAP;

            case ImageViewType::View1DArray:
                ASSERT(!isMultiSampled);
                return GL_TEXTURE_1D_ARRAY;

            case ImageViewType::View2DArray:
                return isMultiSampled ? GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY : GL_TEXTURE_2D_ARRAY;

            case ImageViewType::ViewCubeArray:
                ASSERT(!isMultiSampled);
                return GL_TEXTURE_CUBE_MAP_ARRAY;
            }

            FATAL_ERROR("Invalid texture type");
            return 0;
        }

        uint8_t GetTextureFormatBytesPerPixel(GLenum format)
        {
            switch (format)
            {
            case GL_R8:
            case GL_R8_SNORM:
            case GL_R8I:
            case GL_R8UI:
                return 1;

            case GL_R16:
            case GL_R16_SNORM:
            case GL_R16I:
            case GL_R16UI:
            case GL_RG8:
            case GL_RG8_SNORM:
            case GL_RG8I:
            case GL_RG8UI:
            case GL_R16F:
                return 2;

            case GL_RG16:
            case GL_RG16_SNORM:
            case GL_RG16I:
            case GL_RG16UI:
            case GL_RGBA8:
            case GL_RGBA8_SNORM:
            case GL_RGBA8I:
            case GL_RGBA8UI:
            case GL_R32F:
            case GL_RG16F:
                return 4;

            case GL_RGBA16:
            case GL_RGBA16_SNORM:
            case GL_RGBA16I:
            case GL_RGBA16UI:
            case GL_RG32F:
            case GL_RGBA16F:
                return 8;

            case GL_RGB32F:
                return 12;

            case GL_RGB16F:
                return 6;

            case GL_RGBA32F:
                return 16;

            }

            FATAL_ERROR(base::TempString("Unable to compute per-pixel size of format {}", format));
            return 0;
        }

        GLenum TranslateImageFormat(const ImageFormat format)
        {
            switch (format)
            {
            case ImageFormat::R32F: return GL_R32F;
            case ImageFormat::RG32F: return GL_RG32F;
            case ImageFormat::RGB32F: return GL_RGB32F;
            case ImageFormat::RGBA32F: return GL_RGBA32F;

            case ImageFormat::R32_INT: return GL_R32I;
            case ImageFormat::RG32_INT: return GL_RG32I;
            case ImageFormat::RGB32_INT: return GL_RGB32I;
            case ImageFormat::RGBA32_INT: return GL_RGBA32I;

            case ImageFormat::R32_UINT: return GL_R32UI;
            case ImageFormat::RG32_UINT: return GL_RG32UI;
            case ImageFormat::RGB32_UINT: return GL_RGB32UI;
            case ImageFormat::RGBA32_UINT: return GL_RGBA32UI;

            case ImageFormat::R16F: return GL_R16F;
            case ImageFormat::RG16F: return GL_RG16F;
            case ImageFormat::RGBA16F: return GL_RGBA16F;

            case ImageFormat::R16_INT: return GL_R16I;
            case ImageFormat::RG16_INT: return GL_RG16I;
            case ImageFormat::RGBA16_INT: return GL_RGBA16I;

            case ImageFormat::R16_UINT: return GL_R16UI;
            case ImageFormat::RG16_UINT: return GL_RG16UI;
            case ImageFormat::RGBA16_UINT: return GL_RGBA16UI;

            case ImageFormat::R16_SNORM: return GL_RG16_SNORM;
            case ImageFormat::RG16_SNORM: return GL_RG16_SNORM;
            case ImageFormat::RGBA16_SNORM: return GL_RGBA16_SNORM;

            case ImageFormat::R16_UNORM: return GL_R16;
            case ImageFormat::RG16_UNORM: return GL_RG16;
            case ImageFormat::RGBA16_UNORM: return GL_RGBA16;

            case ImageFormat::R8_UNORM: return GL_R8;
            case ImageFormat::RG8_UNORM: return GL_RG8;
            case ImageFormat::RGB8_UNORM: return GL_RGB8;
            case ImageFormat::RGBA8_UNORM: return GL_RGBA8;

            case ImageFormat::R8_SNORM: return GL_R8_SNORM;
            case ImageFormat::RG8_SNORM: return GL_RG8_SNORM;
            case ImageFormat::RGB8_SNORM: return GL_RGB8_SNORM;
            case ImageFormat::RGBA8_SNORM: return GL_RGBA8_SNORM;

            case ImageFormat::R8_UINT: return GL_R8UI;
            case ImageFormat::RG8_UINT: return GL_RG8UI;
            case ImageFormat::RGB8_UINT: return GL_RGB8UI;
            case ImageFormat::RGBA8_UINT: return GL_RGBA8UI;

            case ImageFormat::R8_INT: return GL_R8I;
            case ImageFormat::RG8_INT: return GL_RG8I;
            case ImageFormat::RGB8_INT: return GL_RGB8I;
            case ImageFormat::RGBA8_INT: return GL_RGBA8I;

            case ImageFormat::BGRA8_UNORM: return GL_BGRA8_EXT;

            case ImageFormat::R11FG11FB10F: return GL_R11F_G11F_B10F;
            case ImageFormat::RGB10_A2_UNORM: return GL_RGB10_A2;
            case ImageFormat::RGB10_A2_UINT: return GL_RGB10_A2UI;
            case ImageFormat::RGBA4_UNORM: return GL_RGBA4;

            case ImageFormat::D24S8: return GL_DEPTH24_STENCIL8;
            case ImageFormat::D24FS8: return GL_DEPTH24_STENCIL8;
            case ImageFormat::D32: return GL_DEPTH32F_STENCIL8;

            case ImageFormat::SRGB8:
                return cvSupportSRGBTexture.get() ? GL_SRGB8 : GL_RGB8;
            case ImageFormat::SRGBA8:
                return cvSupportSRGBTexture.get() ? GL_SRGB8_ALPHA8 : GL_RGBA8;

            case ImageFormat::BC1_UNORM:
                return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;

            case ImageFormat::BC1_SRGB:
                return cvSupportSRGBTexture.get() ? GL_COMPRESSED_SRGB_S3TC_DXT1_EXT : GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;

            case ImageFormat::BC2_UNORM:
                return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;

            case ImageFormat::BC2_SRGB:
                return cvSupportSRGBTexture.get() ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT : GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;

            case ImageFormat::BC3_UNORM:
                return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

            case ImageFormat::BC3_SRGB:
                return cvSupportSRGBTexture.get() ? GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

            case ImageFormat::BC4_UNORM:
                return GL_COMPRESSED_RED_RGTC1;

            case ImageFormat::BC5_UNORM:
                return GL_COMPRESSED_RG_RGTC2;

            case ImageFormat::BC6_SIGNED:
                return GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
            case ImageFormat::BC6_UNSIGNED:
                return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;

            case ImageFormat::BC7_UNORM:
                return GL_COMPRESSED_RGBA_BPTC_UNORM;
            case ImageFormat::BC7_SRGB:
                return GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM;
            }

            TRACE_ERROR("Unsupported rendering image format: {}", (uint32_t)format);
            return 0;
        }

        void DecomposeVertexFormat(GLenum format, GLenum& outBaseFormat, GLuint& outSize, GLboolean& outNormalized)
        {
            switch (format)
            {
                case GL_R32F: outBaseFormat = GL_FLOAT; outSize = 1; outNormalized = GL_FALSE; break;
                case GL_RG32F: outBaseFormat = GL_FLOAT; outSize = 2; outNormalized = GL_FALSE; break;
                case GL_RGB32F: outBaseFormat = GL_FLOAT; outSize = 3; outNormalized = GL_FALSE; break;
                case GL_RGBA32F: outBaseFormat = GL_FLOAT; outSize = 4; outNormalized = GL_FALSE; break;

                case GL_R16F: outBaseFormat = GL_HALF_FLOAT; outSize = 1; outNormalized = GL_FALSE; break;
                case GL_RG16F: outBaseFormat = GL_HALF_FLOAT; outSize = 2; outNormalized = GL_FALSE; break;
                case GL_RGB16F: outBaseFormat = GL_HALF_FLOAT; outSize = 3; outNormalized = GL_FALSE; break;
                case GL_RGBA16F: outBaseFormat = GL_HALF_FLOAT; outSize = 4; outNormalized = GL_FALSE; break;

                case GL_R32I: outBaseFormat = GL_INT; outSize = 1; outNormalized = GL_FALSE; break;
                case GL_RG32I: outBaseFormat = GL_INT; outSize = 2; outNormalized = GL_FALSE; break;
                case GL_RGB32I: outBaseFormat = GL_INT; outSize = 3; outNormalized = GL_FALSE; break;
                case GL_RGBA32I: outBaseFormat = GL_INT; outSize = 4; outNormalized = GL_FALSE; break;

                case GL_R32UI: outBaseFormat = GL_UNSIGNED_INT; outSize = 1; outNormalized = GL_FALSE; break;
                case GL_RG32UI: outBaseFormat = GL_UNSIGNED_INT; outSize = 2; outNormalized = GL_FALSE; break;
                case GL_RGB32UI: outBaseFormat = GL_UNSIGNED_INT; outSize = 3; outNormalized = GL_FALSE; break;
                case GL_RGBA32UI: outBaseFormat = GL_UNSIGNED_INT; outSize = 4; outNormalized = GL_FALSE; break;

                case GL_R16I: outBaseFormat = GL_SHORT; outSize = 1; outNormalized = GL_FALSE; break;
                case GL_RG16I: outBaseFormat = GL_SHORT; outSize = 2; outNormalized = GL_FALSE; break;
                case GL_RGB16I: outBaseFormat = GL_SHORT; outSize = 3; outNormalized = GL_FALSE; break;
                case GL_RGBA16I: outBaseFormat = GL_SHORT; outSize = 4; outNormalized = GL_FALSE; break;

                case GL_R16UI: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 1; outNormalized = GL_FALSE; break;
                case GL_RG16UI: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 2; outNormalized = GL_FALSE; break;
                case GL_RGB16UI: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 3; outNormalized = GL_FALSE; break;
                case GL_RGBA16UI: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 4; outNormalized = GL_FALSE; break;

                case GL_R16: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 1; outNormalized = GL_TRUE; break;
                case GL_RG16: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 2; outNormalized = GL_TRUE; break;
                case GL_RGB16: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 3; outNormalized = GL_TRUE; break;
                case GL_RGBA16: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 4; outNormalized = GL_TRUE; break;

                case GL_R16_SNORM: outBaseFormat = GL_SHORT; outSize = 1; outNormalized = GL_TRUE; break;
                case GL_RG16_SNORM: outBaseFormat = GL_SHORT; outSize = 2; outNormalized = GL_TRUE; break;
                case GL_RGB16_SNORM: outBaseFormat = GL_SHORT; outSize = 3; outNormalized = GL_TRUE; break;
                case GL_RGBA16_SNORM: outBaseFormat = GL_SHORT; outSize = 4; outNormalized = GL_TRUE; break;

                case GL_R8I: outBaseFormat = GL_SHORT; outSize = 1; outNormalized = GL_FALSE; break;
                case GL_RG8I: outBaseFormat = GL_SHORT; outSize = 2; outNormalized = GL_FALSE; break;
                case GL_RGB8I: outBaseFormat = GL_SHORT; outSize = 3; outNormalized = GL_FALSE; break;
                case GL_RGBA8I: outBaseFormat = GL_SHORT; outSize = 4; outNormalized = GL_FALSE; break;

                case GL_R8UI: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 1; outNormalized = GL_FALSE; break;
                case GL_RG8UI: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 2; outNormalized = GL_FALSE; break;
                case GL_RGB8UI: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 3; outNormalized = GL_FALSE; break;
                case GL_RGBA8UI: outBaseFormat = GL_UNSIGNED_SHORT; outSize = 4; outNormalized = GL_FALSE; break;

                case GL_R8: outBaseFormat = GL_UNSIGNED_BYTE; outSize = 1; outNormalized = GL_TRUE; break;
                case GL_RG8: outBaseFormat = GL_UNSIGNED_BYTE; outSize = 2; outNormalized = GL_TRUE; break;
                case GL_RGB8: outBaseFormat = GL_UNSIGNED_BYTE; outSize = 3; outNormalized = GL_TRUE; break;
                case GL_RGBA8: outBaseFormat = GL_UNSIGNED_BYTE; outSize = 4; outNormalized = GL_TRUE; break;

                case GL_R8_SNORM: outBaseFormat = GL_BYTE; outSize = 1; outNormalized = GL_TRUE; break;
                case GL_RG8_SNORM: outBaseFormat = GL_BYTE; outSize = 2; outNormalized = GL_TRUE; break;
                case GL_RGB8_SNORM: outBaseFormat = GL_BYTE; outSize = 3; outNormalized = GL_TRUE; break;
                case GL_RGBA8_SNORM: outBaseFormat = GL_BYTE; outSize = 4; outNormalized = GL_TRUE; break;

                default:
                    FATAL_ERROR("Unknown format!");
            }
        }

        void DecomposeTextureFormat(base::image::PixelFormat format, uint32_t channels, GLenum& outBaseFormat, GLenum& outBaseType)
        {
            switch (format)
            {
            case base::image::PixelFormat::Uint8_Norm:
                switch (channels)
                {
                case 1: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RED; break;
                case 2: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RG; break;
                case 3: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RGB; break;
                case 4: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RGBA; break;
                default:
                    FATAL_ERROR("Unknown channel couint!");
                    break;
                }
                break;
            case base::image::PixelFormat::Uint16_Norm:
                switch (channels)
                {
                case 1: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RED; break;
                case 2: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RG; break;
                case 3: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RGB; break;
                case 4: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RGBA; break;
                default:
                    FATAL_ERROR("Unknown channel couint!");
                    break;
                }
                break;
            case base::image::PixelFormat::Float16_Raw:
                switch (channels)
                {
                case 1: outBaseType = GL_HALF_FLOAT; outBaseFormat = GL_RED; break;
                case 2: outBaseType = GL_HALF_FLOAT; outBaseFormat = GL_RG; break;
                case 3: outBaseType = GL_HALF_FLOAT; outBaseFormat = GL_RGB; break;
                case 4: outBaseType = GL_HALF_FLOAT; outBaseFormat = GL_RGBA; break;
                default:
                    FATAL_ERROR("Unknown channel couint!");
                    break;
                }
                break;
            case base::image::PixelFormat::Float32_Raw:
                switch (channels)
                {
                case 1: outBaseType = GL_FLOAT; outBaseFormat = GL_RED; break;
                case 2: outBaseType = GL_FLOAT; outBaseFormat = GL_RG; break;
                case 3: outBaseType = GL_FLOAT; outBaseFormat = GL_RGB; break;
                case 4: outBaseType = GL_FLOAT; outBaseFormat = GL_RGBA; break;
                default:
                    FATAL_ERROR("Unknown channel couint!");
                    break;
                }
                break;
            default:
                FATAL_ERROR("Unknown format!");
                break;
            }
        }

        void DecomposeTextureFormat(GLenum format, GLenum& outBaseFormat, GLenum& outBaseType, bool& outCompressed)
        {
            outCompressed = false;

            switch (format)
            {
                case GL_R32F: outBaseType = GL_FLOAT; outBaseFormat = GL_RED; break;
                case GL_RG32F: outBaseType = GL_FLOAT; outBaseFormat = GL_RG; break;
                case GL_RGB32F: outBaseType = GL_FLOAT; outBaseFormat = GL_RGB; break;
                case GL_RGBA32F: outBaseType = GL_FLOAT; outBaseFormat = GL_RGBA; break;

                case GL_R16F: outBaseType = GL_HALF_FLOAT; outBaseFormat = GL_RED; break;
                case GL_RG16F: outBaseType = GL_HALF_FLOAT; outBaseFormat = GL_RG; break;
                case GL_RGB16F: outBaseType = GL_HALF_FLOAT; outBaseFormat = GL_RGB; break;
                case GL_RGBA16F: outBaseType = GL_HALF_FLOAT; outBaseFormat = GL_RGBA; break;

                case GL_R32I: outBaseType = GL_INT; outBaseFormat = GL_RED; break;
                case GL_RG32I: outBaseType = GL_INT; outBaseFormat = GL_RG; break;
                case GL_RGB32I: outBaseType = GL_INT; outBaseFormat = GL_RGB; break;
                case GL_RGBA32I: outBaseType = GL_INT; outBaseFormat = GL_RGBA; break;

                case GL_R32UI: outBaseType = GL_UNSIGNED_INT; outBaseFormat = GL_RED; break;
                case GL_RG32UI: outBaseType = GL_UNSIGNED_INT; outBaseFormat = GL_RG; break;
                case GL_RGB32UI: outBaseType = GL_UNSIGNED_INT; outBaseFormat = GL_RGB; break;
                case GL_RGBA32UI: outBaseType = GL_UNSIGNED_INT; outBaseFormat = GL_RGBA; break;

                case GL_R16I: outBaseType = GL_SHORT; outBaseFormat = GL_RED; break;
                case GL_RG16I: outBaseType = GL_SHORT; outBaseFormat = GL_RG; break;
                case GL_RGB16I: outBaseType = GL_SHORT; outBaseFormat = GL_RGB; break;
                case GL_RGBA16I: outBaseType = GL_SHORT; outBaseFormat = GL_RGBA; break;

                case GL_R16UI: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RED; break;
                case GL_RG16UI: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RG; break;
                case GL_RGB16UI: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RGB; break;
                case GL_RGBA16UI: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RGBA; break;

                case GL_R16: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RED; break;
                case GL_RG16: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RG; break;
                case GL_RGB16: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RGB; break;
                case GL_RGBA16: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RGBA; break;

                case GL_R16_SNORM: outBaseType = GL_SHORT; outBaseFormat = GL_RED; break;
                case GL_RG16_SNORM: outBaseType = GL_SHORT; outBaseFormat = GL_RG; break;
                case GL_RGB16_SNORM: outBaseType = GL_SHORT; outBaseFormat = GL_RGB; break;
                case GL_RGBA16_SNORM: outBaseType = GL_SHORT; outBaseFormat = GL_RGBA; break;

                case GL_R8I: outBaseType = GL_SHORT; outBaseFormat = GL_RED; break;
                case GL_RG8I: outBaseType = GL_SHORT; outBaseFormat = GL_RG; break;
                case GL_RGB8I: outBaseType = GL_SHORT; outBaseFormat = GL_RGB; break;
                case GL_RGBA8I: outBaseType = GL_SHORT; outBaseFormat = GL_RGBA; break;

                case GL_R8UI: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RED; break;
                case GL_RG8UI: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RG; break;
                case GL_RGB8UI: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RGB; break;
                case GL_RGBA8UI: outBaseType = GL_UNSIGNED_SHORT; outBaseFormat = GL_RGBA; break;

                case GL_R8: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RED; break;
                case GL_RG8: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RG; break;
                case GL_RGB8: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RGB; break;
                case GL_RGBA8: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RGBA; break;

                case GL_SRGB8: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RGB; break;
                case GL_SRGB8_ALPHA8: outBaseType = GL_UNSIGNED_BYTE; outBaseFormat = GL_RGBA; break;

                case GL_R8_SNORM: outBaseType = GL_BYTE; outBaseFormat = GL_RED; break;
                case GL_RG8_SNORM: outBaseType = GL_BYTE; outBaseFormat = GL_RG; break;
                case GL_RGB8_SNORM: outBaseType = GL_BYTE; outBaseFormat = GL_RGB; break;
                case GL_RGBA8_SNORM: outBaseType = GL_BYTE; outBaseFormat = GL_RGBA; break;

                case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
                case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
                case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
                case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
                case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
                    outBaseType = GL_BYTE;
                    outBaseFormat = GL_RGBA;
                    outCompressed = true;
                    break;

                case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB:
                case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB:
                    outBaseType = GL_HALF_FLOAT;
                    outBaseFormat = GL_RGB;
                    outCompressed = true;
                    break;

                case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
                case GL_COMPRESSED_RGBA_BPTC_UNORM:
                    outBaseType = GL_BYTE;
                    outBaseFormat = GL_RGBA;
                    outCompressed = true;
                    break;

                case GL_COMPRESSED_RED_RGTC1:
                case GL_3DC_X_AMD:
                    outBaseType = GL_BYTE;
                    outBaseFormat = GL_R;
                    outCompressed = true;
                    break;

                case GL_COMPRESSED_RG_RGTC2:
                case GL_3DC_XY_AMD:
                    outBaseType = GL_BYTE;
                    outBaseFormat = GL_RG;
                    outCompressed = true;
                    break;

                case GL_DEPTH32F_STENCIL8:
                case GL_DEPTH24_STENCIL8:
                    outBaseFormat = GL_DEPTH_STENCIL;
                    outBaseType = GL_FLOAT;
                    break;

                case GL_BGRA:
                //case GL_BGRA_EXT:
                case GL_BGRA8_EXT:
                    outBaseType = GL_BYTE; 
                    outBaseFormat = GL_BGRA;
                    break;

                default:
                    FATAL_ERROR("Unknown format!");
            }
        }

    } // gl4
} // driver

#if 0

#include "glUtils.h"

namespace rendering
{
    namespace gl4
    {
            base::StringBuf GetFormatName(VkFormat format)
            {
                #define TEST(x) case x: return base::StringBuf(#x);
                switch (format)
                {
                    TEST(VK_FORMAT_R4G4_UNORM_PACK8);
                    TEST(VK_FORMAT_R4G4B4A4_UNORM_PACK16);
                    TEST(VK_FORMAT_B4G4R4A4_UNORM_PACK16);
                    TEST(VK_FORMAT_R5G6B5_UNORM_PACK16);
                    TEST(VK_FORMAT_B5G6R5_UNORM_PACK16);
                    TEST(VK_FORMAT_R5G5B5A1_UNORM_PACK16);
                    TEST(VK_FORMAT_B5G5R5A1_UNORM_PACK16);
                    TEST(VK_FORMAT_A1R5G5B5_UNORM_PACK16);
                    TEST(VK_FORMAT_R8_UNORM);
                    TEST(VK_FORMAT_R8_SNORM);
                    TEST(VK_FORMAT_R8_USCALED);
                    TEST(VK_FORMAT_R8_SSCALED);
                    TEST(VK_FORMAT_R8_UINT);
                    TEST(VK_FORMAT_R8_SINT);
                    TEST(VK_FORMAT_R8_SRGB);
                    TEST(VK_FORMAT_R8G8_UNORM);
                    TEST(VK_FORMAT_R8G8_SNORM);
                    TEST(VK_FORMAT_R8G8_USCALED);
                    TEST(VK_FORMAT_R8G8_SSCALED);
                    TEST(VK_FORMAT_R8G8_UINT);
                    TEST(VK_FORMAT_R8G8_SINT);
                    TEST(VK_FORMAT_R8G8_SRGB);
                    TEST(VK_FORMAT_R8G8B8_UNORM);
                    TEST(VK_FORMAT_R8G8B8_SNORM);
                    TEST(VK_FORMAT_R8G8B8_USCALED);
                    TEST(VK_FORMAT_R8G8B8_SSCALED);
                    TEST(VK_FORMAT_R8G8B8_UINT);
                    TEST(VK_FORMAT_R8G8B8_SINT);
                    TEST(VK_FORMAT_R8G8B8_SRGB);
                    TEST(VK_FORMAT_B8G8R8_UNORM);
                    TEST(VK_FORMAT_B8G8R8_SNORM);
                    TEST(VK_FORMAT_B8G8R8_USCALED);
                    TEST(VK_FORMAT_B8G8R8_SSCALED);
                    TEST(VK_FORMAT_B8G8R8_UINT);
                    TEST(VK_FORMAT_B8G8R8_SINT);
                    TEST(VK_FORMAT_B8G8R8_SRGB);
                    TEST(VK_FORMAT_R8G8B8A8_UNORM);
                    TEST(VK_FORMAT_R8G8B8A8_SNORM);
                    TEST(VK_FORMAT_R8G8B8A8_USCALED);
                    TEST(VK_FORMAT_R8G8B8A8_SSCALED);
                    TEST(VK_FORMAT_R8G8B8A8_UINT);
                    TEST(VK_FORMAT_R8G8B8A8_SINT);
                    TEST(VK_FORMAT_R8G8B8A8_SRGB);
                    TEST(VK_FORMAT_B8G8R8A8_UNORM);
                    TEST(VK_FORMAT_B8G8R8A8_SNORM);
                    TEST(VK_FORMAT_B8G8R8A8_USCALED);
                    TEST(VK_FORMAT_B8G8R8A8_SSCALED);
                    TEST(VK_FORMAT_B8G8R8A8_UINT);
                    TEST(VK_FORMAT_B8G8R8A8_SINT);
                    TEST(VK_FORMAT_B8G8R8A8_SRGB);
                    TEST(VK_FORMAT_A8B8G8R8_UNORM_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_SNORM_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_USCALED_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_SSCALED_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_UINT_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_SINT_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_SRGB_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_UNORM_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_SNORM_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_USCALED_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_SSCALED_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_UINT_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_SINT_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_SNORM_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_USCALED_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_SSCALED_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_UINT_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_SINT_PACK32);
                    TEST(VK_FORMAT_R16_UNORM);
                    TEST(VK_FORMAT_R16_SNORM);
                    TEST(VK_FORMAT_R16_USCALED);
                    TEST(VK_FORMAT_R16_SSCALED);
                    TEST(VK_FORMAT_R16_UINT);
                    TEST(VK_FORMAT_R16_SINT);
                    TEST(VK_FORMAT_R16_SFLOAT);
                    TEST(VK_FORMAT_R16G16_UNORM);
                    TEST(VK_FORMAT_R16G16_SNORM);
                    TEST(VK_FORMAT_R16G16_USCALED);
                    TEST(VK_FORMAT_R16G16_SSCALED);
                    TEST(VK_FORMAT_R16G16_UINT);
                    TEST(VK_FORMAT_R16G16_SINT);
                    TEST(VK_FORMAT_R16G16_SFLOAT);
                    TEST(VK_FORMAT_R16G16B16_UNORM);
                    TEST(VK_FORMAT_R16G16B16_SNORM);
                    TEST(VK_FORMAT_R16G16B16_USCALED);
                    TEST(VK_FORMAT_R16G16B16_SSCALED);
                    TEST(VK_FORMAT_R16G16B16_UINT);
                    TEST(VK_FORMAT_R16G16B16_SINT);
                    TEST(VK_FORMAT_R16G16B16_SFLOAT);
                    TEST(VK_FORMAT_R16G16B16A16_UNORM);
                    TEST(VK_FORMAT_R16G16B16A16_SNORM);
                    TEST(VK_FORMAT_R16G16B16A16_USCALED);
                    TEST(VK_FORMAT_R16G16B16A16_SSCALED);
                    TEST(VK_FORMAT_R16G16B16A16_UINT);
                    TEST(VK_FORMAT_R16G16B16A16_SINT);
                    TEST(VK_FORMAT_R16G16B16A16_SFLOAT);
                    TEST(VK_FORMAT_R32_UINT);
                    TEST(VK_FORMAT_R32_SINT);
                    TEST(VK_FORMAT_R32_SFLOAT);
                    TEST(VK_FORMAT_R32G32_UINT);
                    TEST(VK_FORMAT_R32G32_SINT);
                    TEST(VK_FORMAT_R32G32_SFLOAT);
                    TEST(VK_FORMAT_R32G32B32_UINT);
                    TEST(VK_FORMAT_R32G32B32_SINT);
                    TEST(VK_FORMAT_R32G32B32_SFLOAT);
                    TEST(VK_FORMAT_R32G32B32A32_UINT);
                    TEST(VK_FORMAT_R32G32B32A32_SINT);
                    TEST(VK_FORMAT_R32G32B32A32_SFLOAT);
                    TEST(VK_FORMAT_R64_UINT);
                    TEST(VK_FORMAT_R64_SINT);
                    TEST(VK_FORMAT_R64_SFLOAT);
                    TEST(VK_FORMAT_R64G64_UINT);
                    TEST(VK_FORMAT_R64G64_SINT);
                    TEST(VK_FORMAT_R64G64_SFLOAT);
                    TEST(VK_FORMAT_R64G64B64_UINT);
                    TEST(VK_FORMAT_R64G64B64_SINT);
                    TEST(VK_FORMAT_R64G64B64_SFLOAT);
                    TEST(VK_FORMAT_R64G64B64A64_UINT);
                    TEST(VK_FORMAT_R64G64B64A64_SINT);
                    TEST(VK_FORMAT_R64G64B64A64_SFLOAT);
                    TEST(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
                    TEST(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
                    TEST(VK_FORMAT_D16_UNORM);
                    TEST(VK_FORMAT_X8_D24_UNORM_PACK32);
                    TEST(VK_FORMAT_D32_SFLOAT);
                    TEST(VK_FORMAT_S8_UINT);
                    TEST(VK_FORMAT_D16_UNORM_S8_UINT);
                    TEST(VK_FORMAT_D24_UNORM_S8_UINT);
                    TEST(VK_FORMAT_D32_SFLOAT_S8_UINT);
                    TEST(VK_FORMAT_BC1_RGB_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC1_RGB_SRGB_BLOCK);
                    TEST(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
                    TEST(VK_FORMAT_BC2_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC2_SRGB_BLOCK);
                    TEST(VK_FORMAT_BC3_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC3_SRGB_BLOCK);
                    TEST(VK_FORMAT_BC4_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC4_SNORM_BLOCK);
                    TEST(VK_FORMAT_BC5_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC5_SNORM_BLOCK);
                    TEST(VK_FORMAT_BC6H_UFLOAT_BLOCK);
                    TEST(VK_FORMAT_BC6H_SFLOAT_BLOCK);
                    TEST(VK_FORMAT_BC7_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC7_SRGB_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
                    TEST(VK_FORMAT_EAC_R11_UNORM_BLOCK);
                    TEST(VK_FORMAT_EAC_R11_SNORM_BLOCK);
                    TEST(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
                    TEST(VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
                    TEST(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG);
                }
                #undef TEST

                return base::StringBuf("Unknown");
            }

            bool IsDepthFormat(VkFormat format)
            {
                switch (format)
                {
                    case VK_FORMAT_D16_UNORM:
                    case VK_FORMAT_X8_D24_UNORM_PACK32:
                    case VK_FORMAT_D32_SFLOAT:
                    case VK_FORMAT_D16_UNORM_S8_UINT:
                    case VK_FORMAT_D24_UNORM_S8_UINT:
                    case VK_FORMAT_D32_SFLOAT_S8_UINT:
                        return true;
                }

                return false;
            }

            bool IsStencilFormat(VkFormat format)
            {
                switch (format)
                {
                    case VK_FORMAT_X8_D24_UNORM_PACK32:
                    case VK_FORMAT_S8_UINT:
                    case VK_FORMAT_D16_UNORM_S8_UINT:
                    case VK_FORMAT_D24_UNORM_S8_UINT:
                    case VK_FORMAT_D32_SFLOAT_S8_UINT:
                        return true;
                }

                return false;
            }

            VkFormat FindFormatForName(const base::StringBuf& name)
            {
                static base::HashMap<base::StringBuf, VkFormat> formats;

                if (formats.empty())
                {
#define TEST(x) formats.set( base::StringBuf(#x), x);
                    TEST(VK_FORMAT_R4G4_UNORM_PACK8);
                    TEST(VK_FORMAT_R4G4B4A4_UNORM_PACK16);
                    TEST(VK_FORMAT_B4G4R4A4_UNORM_PACK16);
                    TEST(VK_FORMAT_R5G6B5_UNORM_PACK16);
                    TEST(VK_FORMAT_B5G6R5_UNORM_PACK16);
                    TEST(VK_FORMAT_R5G5B5A1_UNORM_PACK16);
                    TEST(VK_FORMAT_B5G5R5A1_UNORM_PACK16);
                    TEST(VK_FORMAT_A1R5G5B5_UNORM_PACK16);
                    TEST(VK_FORMAT_R8_UNORM);
                    TEST(VK_FORMAT_R8_SNORM);
                    TEST(VK_FORMAT_R8_USCALED);
                    TEST(VK_FORMAT_R8_SSCALED);
                    TEST(VK_FORMAT_R8_UINT);
                    TEST(VK_FORMAT_R8_SINT);
                    TEST(VK_FORMAT_R8_SRGB);
                    TEST(VK_FORMAT_R8G8_UNORM);
                    TEST(VK_FORMAT_R8G8_SNORM);
                    TEST(VK_FORMAT_R8G8_USCALED);
                    TEST(VK_FORMAT_R8G8_SSCALED);
                    TEST(VK_FORMAT_R8G8_UINT);
                    TEST(VK_FORMAT_R8G8_SINT);
                    TEST(VK_FORMAT_R8G8_SRGB);
                    TEST(VK_FORMAT_R8G8B8_UNORM);
                    TEST(VK_FORMAT_R8G8B8_SNORM);
                    TEST(VK_FORMAT_R8G8B8_USCALED);
                    TEST(VK_FORMAT_R8G8B8_SSCALED);
                    TEST(VK_FORMAT_R8G8B8_UINT);
                    TEST(VK_FORMAT_R8G8B8_SINT);
                    TEST(VK_FORMAT_R8G8B8_SRGB);
                    TEST(VK_FORMAT_B8G8R8_UNORM);
                    TEST(VK_FORMAT_B8G8R8_SNORM);
                    TEST(VK_FORMAT_B8G8R8_USCALED);
                    TEST(VK_FORMAT_B8G8R8_SSCALED);
                    TEST(VK_FORMAT_B8G8R8_UINT);
                    TEST(VK_FORMAT_B8G8R8_SINT);
                    TEST(VK_FORMAT_B8G8R8_SRGB);
                    TEST(VK_FORMAT_R8G8B8A8_UNORM);
                    TEST(VK_FORMAT_R8G8B8A8_SNORM);
                    TEST(VK_FORMAT_R8G8B8A8_USCALED);
                    TEST(VK_FORMAT_R8G8B8A8_SSCALED);
                    TEST(VK_FORMAT_R8G8B8A8_UINT);
                    TEST(VK_FORMAT_R8G8B8A8_SINT);
                    TEST(VK_FORMAT_R8G8B8A8_SRGB);
                    TEST(VK_FORMAT_B8G8R8A8_UNORM);
                    TEST(VK_FORMAT_B8G8R8A8_SNORM);
                    TEST(VK_FORMAT_B8G8R8A8_USCALED);
                    TEST(VK_FORMAT_B8G8R8A8_SSCALED);
                    TEST(VK_FORMAT_B8G8R8A8_UINT);
                    TEST(VK_FORMAT_B8G8R8A8_SINT);
                    TEST(VK_FORMAT_B8G8R8A8_SRGB);
                    TEST(VK_FORMAT_A8B8G8R8_UNORM_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_SNORM_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_USCALED_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_SSCALED_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_UINT_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_SINT_PACK32);
                    TEST(VK_FORMAT_A8B8G8R8_SRGB_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_UNORM_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_SNORM_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_USCALED_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_SSCALED_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_UINT_PACK32);
                    TEST(VK_FORMAT_A2R10G10B10_SINT_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_SNORM_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_USCALED_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_SSCALED_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_UINT_PACK32);
                    TEST(VK_FORMAT_A2B10G10R10_SINT_PACK32);
                    TEST(VK_FORMAT_R16_UNORM);
                    TEST(VK_FORMAT_R16_SNORM);
                    TEST(VK_FORMAT_R16_USCALED);
                    TEST(VK_FORMAT_R16_SSCALED);
                    TEST(VK_FORMAT_R16_UINT);
                    TEST(VK_FORMAT_R16_SINT);
                    TEST(VK_FORMAT_R16_SFLOAT);
                    TEST(VK_FORMAT_R16G16_UNORM);
                    TEST(VK_FORMAT_R16G16_SNORM);
                    TEST(VK_FORMAT_R16G16_USCALED);
                    TEST(VK_FORMAT_R16G16_SSCALED);
                    TEST(VK_FORMAT_R16G16_UINT);
                    TEST(VK_FORMAT_R16G16_SINT);
                    TEST(VK_FORMAT_R16G16_SFLOAT);
                    TEST(VK_FORMAT_R16G16B16_UNORM);
                    TEST(VK_FORMAT_R16G16B16_SNORM);
                    TEST(VK_FORMAT_R16G16B16_USCALED);
                    TEST(VK_FORMAT_R16G16B16_SSCALED);
                    TEST(VK_FORMAT_R16G16B16_UINT);
                    TEST(VK_FORMAT_R16G16B16_SINT);
                    TEST(VK_FORMAT_R16G16B16_SFLOAT);
                    TEST(VK_FORMAT_R16G16B16A16_UNORM);
                    TEST(VK_FORMAT_R16G16B16A16_SNORM);
                    TEST(VK_FORMAT_R16G16B16A16_USCALED);
                    TEST(VK_FORMAT_R16G16B16A16_SSCALED);
                    TEST(VK_FORMAT_R16G16B16A16_UINT);
                    TEST(VK_FORMAT_R16G16B16A16_SINT);
                    TEST(VK_FORMAT_R16G16B16A16_SFLOAT);
                    TEST(VK_FORMAT_R32_UINT);
                    TEST(VK_FORMAT_R32_SINT);
                    TEST(VK_FORMAT_R32_SFLOAT);
                    TEST(VK_FORMAT_R32G32_UINT);
                    TEST(VK_FORMAT_R32G32_SINT);
                    TEST(VK_FORMAT_R32G32_SFLOAT);
                    TEST(VK_FORMAT_R32G32B32_UINT);
                    TEST(VK_FORMAT_R32G32B32_SINT);
                    TEST(VK_FORMAT_R32G32B32_SFLOAT);
                    TEST(VK_FORMAT_R32G32B32A32_UINT);
                    TEST(VK_FORMAT_R32G32B32A32_SINT);
                    TEST(VK_FORMAT_R32G32B32A32_SFLOAT);
                    TEST(VK_FORMAT_R64_UINT);
                    TEST(VK_FORMAT_R64_SINT);
                    TEST(VK_FORMAT_R64_SFLOAT);
                    TEST(VK_FORMAT_R64G64_UINT);
                    TEST(VK_FORMAT_R64G64_SINT);
                    TEST(VK_FORMAT_R64G64_SFLOAT);
                    TEST(VK_FORMAT_R64G64B64_UINT);
                    TEST(VK_FORMAT_R64G64B64_SINT);
                    TEST(VK_FORMAT_R64G64B64_SFLOAT);
                    TEST(VK_FORMAT_R64G64B64A64_UINT);
                    TEST(VK_FORMAT_R64G64B64A64_SINT);
                    TEST(VK_FORMAT_R64G64B64A64_SFLOAT);
                    TEST(VK_FORMAT_B10G11R11_UFLOAT_PACK32);
                    TEST(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32);
                    TEST(VK_FORMAT_D16_UNORM);
                    TEST(VK_FORMAT_X8_D24_UNORM_PACK32);
                    TEST(VK_FORMAT_D32_SFLOAT);
                    TEST(VK_FORMAT_S8_UINT);
                    TEST(VK_FORMAT_D16_UNORM_S8_UINT);
                    TEST(VK_FORMAT_D24_UNORM_S8_UINT);
                    TEST(VK_FORMAT_D32_SFLOAT_S8_UINT);
                    TEST(VK_FORMAT_BC1_RGB_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC1_RGB_SRGB_BLOCK);
                    TEST(VK_FORMAT_BC1_RGBA_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC1_RGBA_SRGB_BLOCK);
                    TEST(VK_FORMAT_BC2_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC2_SRGB_BLOCK);
                    TEST(VK_FORMAT_BC3_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC3_SRGB_BLOCK);
                    TEST(VK_FORMAT_BC4_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC4_SNORM_BLOCK);
                    TEST(VK_FORMAT_BC5_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC5_SNORM_BLOCK);
                    TEST(VK_FORMAT_BC6H_UFLOAT_BLOCK);
                    TEST(VK_FORMAT_BC6H_SFLOAT_BLOCK);
                    TEST(VK_FORMAT_BC7_UNORM_BLOCK);
                    TEST(VK_FORMAT_BC7_SRGB_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
                    TEST(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK);
                    TEST(VK_FORMAT_EAC_R11_UNORM_BLOCK);
                    TEST(VK_FORMAT_EAC_R11_SNORM_BLOCK);
                    TEST(VK_FORMAT_EAC_R11G11_UNORM_BLOCK);
                    TEST(VK_FORMAT_EAC_R11G11_SNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_4x4_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_5x4_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_5x4_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_5x5_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_5x5_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_6x5_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_6x5_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_6x6_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_6x6_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x5_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x5_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x6_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x6_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x8_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_8x8_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x5_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x5_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x6_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x6_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x8_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x8_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x10_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_10x10_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_12x10_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_12x10_SRGB_BLOCK);
                    TEST(VK_FORMAT_ASTC_12x12_UNORM_BLOCK);
                    TEST(VK_FORMAT_ASTC_12x12_SRGB_BLOCK);
                    TEST(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG);
                    TEST(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG);
#undef TEST
                }

                VkFormat format = VK_FORMAT_UNDEFINED;
                formats.find(name, format);
                return format;
            }

            base::StringBuf GetPresentationModeName(VkPresentModeKHR mode)
            {
                switch (mode)
                {
                    case VK_PRESENT_MODE_IMMEDIATE_KHR: return base::StringBuf("VK_PRESENT_MODE_IMMEDIATE_KHR");
                    case VK_PRESENT_MODE_MAILBOX_KHR: return base::StringBuf("VK_PRESENT_MODE_MAILBOX_KHR");
                    case VK_PRESENT_MODE_FIFO_KHR: return base::StringBuf("VK_PRESENT_MODE_FIFO_KHR");
                    case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return base::StringBuf("VK_PRESENT_MODE_FIFO_RELAXED_KHR");
                }

                return base::StringBuf("Unknown");
            }

            base::StringBuf GetAttachmentDescriptionFlagsName(VkAttachmentDescriptionFlags flags)
            {
                base::StringBuf ret = "";
                if (flags & VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT) ret += "VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT";
                return ret;
            }

            base::StringBuf GetSampleCountFlagBitsName(VkSampleCountFlagBits flags)
            {
                base::StringBuf ret = "";
                if (flags & VK_SAMPLE_COUNT_1_BIT) ret += "VK_SAMPLE_COUNT_1_BIT";
                if (flags & VK_SAMPLE_COUNT_2_BIT) ret += "VK_SAMPLE_COUNT_2_BIT";
                if (flags & VK_SAMPLE_COUNT_4_BIT) ret += "VK_SAMPLE_COUNT_4_BIT";
                if (flags & VK_SAMPLE_COUNT_8_BIT) ret += "VK_SAMPLE_COUNT_8_BIT";
                if (flags & VK_SAMPLE_COUNT_16_BIT) ret += "VK_SAMPLE_COUNT_16_BIT";
                if (flags & VK_SAMPLE_COUNT_32_BIT) ret += "VK_SAMPLE_COUNT_32_BIT";
                if (flags & VK_SAMPLE_COUNT_64_BIT) ret += "VK_SAMPLE_COUNT_64_BIT";
                return ret;
            }

            base::StringBuf GetAttachmentLoadOpName(VkAttachmentLoadOp val)
            {
                switch (val)
                {
                    case VK_ATTACHMENT_LOAD_OP_LOAD: return "VK_ATTACHMENT_LOAD_OP_LOAD";
                    case VK_ATTACHMENT_LOAD_OP_CLEAR: return "VK_ATTACHMENT_LOAD_OP_CLEAR";
                    case VK_ATTACHMENT_LOAD_OP_DONT_CARE: return "VK_ATTACHMENT_LOAD_OP_DONT_CARE";
                }

                return base::StringBuf("Unknown");
            }

            base::StringBuf GetAttachmentStoreOpName(VkAttachmentStoreOp val)
            {
                switch (val)
                {
                    case VK_ATTACHMENT_STORE_OP_STORE: return base::StringBuf("VK_ATTACHMENT_STORE_OP_STORE");
                    case VK_ATTACHMENT_STORE_OP_DONT_CARE: return base::StringBuf("VK_ATTACHMENT_STORE_OP_DONT_CARE");
                }

                return base::StringBuf("Unknown");
            }           

            base::StringBuf GetImageLayoutName(VkImageLayout val)
            {
                switch (val)
                {
                    case VK_IMAGE_LAYOUT_UNDEFINED: return base::StringBuf("VK_IMAGE_LAYOUT_UNDEFINED");
                    case VK_IMAGE_LAYOUT_GENERAL: return base::StringBuf("VK_IMAGE_LAYOUT_GENERAL");
                    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return base::StringBuf("VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL");
                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return base::StringBuf("VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL");
                    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: return base::StringBuf("VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL");
                    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return base::StringBuf("VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL");
                    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return base::StringBuf("VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL");
                    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return base::StringBuf("VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL");
                    case VK_IMAGE_LAYOUT_PREINITIALIZED: return base::StringBuf("VK_IMAGE_LAYOUT_PREINITIALIZED");
                    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return base::StringBuf("VK_IMAGE_LAYOUT_PRESENT_SRC_KHR");
                }

                return base::StringBuf("Unknown");
            }

            base::StringBuf GetDescriptorTypeName(VkDescriptorType val)
            {
                switch (val)
                {
                    case VK_DESCRIPTOR_TYPE_SAMPLER: return base::StringBuf("VK_DESCRIPTOR_TYPE_SAMPLER");
                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return base::StringBuf("VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER");
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return base::StringBuf("VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE");
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: return base::StringBuf("VK_DESCRIPTOR_TYPE_STORAGE_IMAGE");
                    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return base::StringBuf("VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER");
                    case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return base::StringBuf("VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER");
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return base::StringBuf("VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER");
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: return base::StringBuf("VK_DESCRIPTOR_TYPE_STORAGE_BUFFER");
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return base::StringBuf("VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC");
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return base::StringBuf("VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC");
                    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return base::StringBuf("VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT");
                }

                return base::StringBuf("Unknown");
            }

            //---


            //---

            VkAccessFlags TranslatePipelineDependencyAccessFlags(const InputDependency dep)
            {
                switch (dep)
                {
                    case InputDependency::VertexInput: 
                        return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;

                    case InputDependency::PixelShaderInput:
                    case InputDependency::VertexShaderInput:
                        return VK_ACCESS_SHADER_READ_BIT;

                    case InputDependency::RasterInput:
                        return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                }

                ASSERT("!Invalid value");
                return 0;
            }

            VkAccessFlags TranslatePipelineDependencyAccessFlags(const OutputDependency dep)
            {
                switch (dep)
                {
                    case OutputDependency::ShaderOutput:
                        return VK_ACCESS_SHADER_WRITE_BIT;

                    case OutputDependency::RasterOutput:
                        return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                }

                ASSERT("!Invalid value");
                return 0;
            }

            VkPipelineStageFlags TranslatePipelineDependencyStageFlags(const InputDependency dep)
            {
                switch (dep)
                {
                    case InputDependency::VertexInput:
                        return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

                    case InputDependency::VertexShaderInput:
                        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;

                    case InputDependency::PixelShaderInput:
                        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

                    case InputDependency::RasterInput:
                        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                }

                ASSERT("!Invalid value");
                return 0;
            }

            VkPipelineStageFlags TranslatePipelineDependencyStageFlags(const OutputDependency dep)
            {
                switch (dep)
                {
                    case OutputDependency::ShaderOutput:
                        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;

                    case OutputDependency::RasterOutput:
                        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                }

                ASSERT("!Invalid value");
                return 0;
            }

    } // gl4
} // driver

#endif