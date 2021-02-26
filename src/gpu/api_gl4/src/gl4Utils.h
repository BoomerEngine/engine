/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//--

// get number of bits per pixel of texture
extern GPU_API_GL4_API uint8_t GetTextureFormatBytesPerPixel(GLenum format);

// get the view type
extern GPU_API_GL4_API GLenum TranslateTextureType(ImageViewType viewType, bool isMultiSampled = false);

// translate image format
extern GPU_API_GL4_API GLuint TranslateImageFormat(const ImageFormat format);

// decompose format into parts
extern GPU_API_GL4_API void DecomposeVertexFormat(GLenum format, GLenum& outBaseFormat, GLuint& outSize, GLboolean& outNormalized);

// decompose format into parts
extern GPU_API_GL4_API void DecomposeTextureFormat(GLenum format, GLenum& outBaseFormat, GLenum& outBaseType, bool* outCompressed = nullptr);

// decompose format into parts
extern GPU_API_GL4_API void DecomposeTextureFormat(image::PixelFormat format, uint32_t channels, GLenum& outBaseFormat, GLenum& outBaseType);

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
