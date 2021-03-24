/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "engine/texture/include/staticTexture.h"
#include "core/object/include/compressedBuffer.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

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
struct IMPORT_TEXTURE_LOADER_API ImageCompressionSettings
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(ImageCompressionSettings);

public:
    ImageCompressionSettings();

    StringBuf m_suffix;

    ImageContentType m_contentType = ImageContentType::Auto;
    ImageContentColorSpace m_contentColorSpace = ImageContentColorSpace::Auto;
    ImageAlphaMode m_contentAlphaMode = ImageAlphaMode::NotPremultiplied;

    ImageMipmapGenerationMode m_mipmapMode = ImageMipmapGenerationMode::Auto;

    ImageCompressionFormat m_compressionMode = ImageCompressionFormat::Auto;
    ImageValidPixelsMaskingMode m_compressionMasking = ImageValidPixelsMaskingMode::Auto;
    ImageCompressionQuality m_compressionQuality = ImageCompressionQuality::Normal;

    bool m_compressDataBuffer = true;
};

//--

struct IMPORT_TEXTURE_LOADER_API ImageCompressedResult : public IReferencable
{
    CompressedBufer data;
    Array<StaticTextureMip> mips;
    TextureInfo info;

    //gpu::ImageObjectPtr createPreviewTexture() const;
};

//--

/// determine storage size needed for compressed data
extern IMPORT_TEXTURE_LOADER_API uint32_t CalcCompressedImageDataSize(const ImageView& data, ImageCompressionFormat format);

/// bake data for compressed texture (single slice)
extern IMPORT_TEXTURE_LOADER_API RefPtr<ImageCompressedResult> CompressImage(const ImageView& data, const ImageCompressionSettings& settings, IProgressTracker& progress);

/// merge multiple compressed slices 
extern IMPORT_TEXTURE_LOADER_API RefPtr<ImageCompressedResult> MergeCompressedImages(const Array<RefPtr<ImageCompressedResult>>& slices, ImageViewType viewType, bool compressFinalDataBuffer);

//--

END_BOOMER_NAMESPACE_EX(assets)
