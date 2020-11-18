/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "rendering/texture/include/renderingStaticTexture.h"

namespace rendering
{

    //--

    enum class ImageContentType : uint8_t
    {
        Auto,
        Generic,
        Albedo,
        Specularity,
        Metalness,
        Mask,
        Roughness,
        Bumpmap,
        CombinedMetallicSmoothness, // R-metallic, A-smoothness (1-roughness)
        CombinedRoughnessSpecularity,
        TangentNormalMap,
        WorldNormalMap,
        AmbientOcclusion,
        Emissive,
    };

    enum class ImageMipmapGenerationMode : uint8_t
    {
        Auto,
        None,
        BoxFilter,
    };

    enum class ImageContentColorSpace : uint8_t
    {
        Auto,
        Linear,
        SRGB,
        Normals,
        HDR, // positive values only!
    };

    enum class ImageCompressionFormat : uint8_t
    {
        Auto,
        None,
        BC1,
        BC2,
        BC3,
        BC4,
        BC5,
        BC6H,
        BC7,
    };

    enum class ImageValidPixelsMaskingMode : uint8_t
    {
        Auto,
        None, // all pixels contribute
        ByAlpha, // only pixels with non zero alpha contribute - default if image has alpha channel
        NonBlack, // only pixels that are not 0,0,0 contribute (great for normal maps)
    };

    enum class ImageCompressionQuality : uint8_t
    {
        Quick,
        Normal,
        Placebo,
    };

    enum class ImageAlphaMode : uint8_t
    {
        NotPremultiplied,
        AlreadyPremultiplied,
        Premultiply,
        RemoveAlpha,
    };

    /// settings for texture compression stuff
    struct ASSETS_IMAGE_TEXTURE_API ImageCompressionSettings
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(ImageCompressionSettings);

    public:
        ImageCompressionSettings();

        base::StringBuf m_suffix;

        ImageContentType m_contentType = ImageContentType::Auto;
        ImageContentColorSpace m_contentColorSpace = ImageContentColorSpace::Auto;
        ImageAlphaMode m_contentAlphaMode = ImageAlphaMode::NotPremultiplied;

        ImageMipmapGenerationMode m_mipmapMode = ImageMipmapGenerationMode::Auto;

        ImageCompressionFormat m_compressionMode = ImageCompressionFormat::Auto;
        ImageValidPixelsMaskingMode m_compressionMasking = ImageValidPixelsMaskingMode::Auto;
        ImageCompressionQuality m_compressionQuality = ImageCompressionQuality::Normal;
    };

    //--

    struct ASSETS_IMAGE_TEXTURE_API ImageCompressedResult : public base::IReferencable
    {
        base::Buffer data;
        base::Array<StaticTextureMip> mips;
        TextureInfo info;

        ImageObjectPtr createPreviewTexture() const;
    };

    //--

    /// determine storage size needed for compressed data
    extern ASSETS_IMAGE_TEXTURE_API uint32_t CalcCompressedImageDataSize(const base::image::ImageView& data, ImageCompressionFormat format);

    /// bake data for compressed texture (single slice)
    extern ASSETS_IMAGE_TEXTURE_API base::RefPtr<ImageCompressedResult> CompressImage(const base::image::ImageView& data, const ImageCompressionSettings& settings, base::IProgressTracker& progress);

    //--

} // rendering