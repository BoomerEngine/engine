/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: utils #]
***/

#include "build.h"
#include "gl4Utils.h"

namespace rendering
{
	namespace api
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

			void DecomposeTextureFormat(GLenum format, GLenum& outBaseFormat, GLenum& outBaseType, bool* outCompressed)
			{
				bool compressed = false;

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
					compressed = true;
					break;

				case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB:
				case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB:
					outBaseType = GL_HALF_FLOAT;
					outBaseFormat = GL_RGB;
					compressed = true;
					break;

				case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM:
				case GL_COMPRESSED_RGBA_BPTC_UNORM:
					outBaseType = GL_BYTE;
					outBaseFormat = GL_RGBA;
					compressed = true;
					break;

				case GL_COMPRESSED_RED_RGTC1:
				case GL_3DC_X_AMD:
					outBaseType = GL_BYTE;
					outBaseFormat = GL_R;
					compressed = true;
					break;

				case GL_COMPRESSED_RG_RGTC2:
				case GL_3DC_XY_AMD:
					outBaseType = GL_BYTE;
					outBaseFormat = GL_RG;
					compressed = true;
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

				if (outCompressed)
					*outCompressed = compressed;
			}

		} // gl4
	} // api
} // rendering
