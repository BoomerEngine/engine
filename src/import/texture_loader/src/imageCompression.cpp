/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "imageCompression.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"
#include "core/image/include/imageUtils.h"

#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"

#include "squish/squish.h"
#include "bc6h/BC6H_Encode.h"
#include "bc7/BC7_Encode.h"
#include "bc7/BC7_Definitions.h"
#include "bc7/BC7_Library.h"
#include "bc7/shake.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

ConfigProperty<int> cvMaxTextureWidth("Rendering.Texture", "MaxTextureWidth", 32768);// 8192);
ConfigProperty<int> cvMaxTextureHeight("Rendering.Texture", "MaxTextureHeight", 32768);// 8192);
ConfigProperty<int> cvMaxTextureDepth("Rendering.Texture", "MaxTextureDepth", 2048);
ConfigProperty<int> cvMaxTextureDataSizeMB("Rendering.Texture", "MaxTextureDataSizeMB", 2048);

ConfigProperty<int> cvMaxTextureCompressionThreads("Rendering.Texture", "MaxTextureCompressionThreads", -2); // all possible but keep 2 on side
ConfigProperty<int> cvTextureCompressionBlockSize("Rendering.Texture", "TextureCompressionBlockSize", 4); // 4x4 blocks - 16x16 pixels

//--

RTTI_BEGIN_TYPE_ENUM(ImageContentType);
    RTTI_ENUM_OPTION(Auto);
    RTTI_ENUM_OPTION(Generic);
    RTTI_ENUM_OPTION(Albedo);
    RTTI_ENUM_OPTION(Specularity);
    RTTI_ENUM_OPTION(Metalness);
    RTTI_ENUM_OPTION(Roughness);
    RTTI_ENUM_OPTION(Mask);
    RTTI_ENUM_OPTION(Bumpmap);
    RTTI_ENUM_OPTION(CombinedMetallicSmoothness);
    RTTI_ENUM_OPTION(CombinedRoughnessSpecularity);
    RTTI_ENUM_OPTION(TangentNormalMap);
    RTTI_ENUM_OPTION(WorldNormalMap);
    RTTI_ENUM_OPTION(AmbientOcclusion);
    RTTI_ENUM_OPTION(Emissive);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(ImageContentColorSpace);
    RTTI_ENUM_OPTION(Auto);
    RTTI_ENUM_OPTION(Linear);
    RTTI_ENUM_OPTION(SRGB);
    RTTI_ENUM_OPTION(Normals);
    RTTI_ENUM_OPTION(HDR);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(ImageMipmapGenerationMode);
    RTTI_ENUM_OPTION(Auto);
    RTTI_ENUM_OPTION(None);
    RTTI_ENUM_OPTION(BoxFilter);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(ImageValidPixelsMaskingMode);
    RTTI_ENUM_OPTION(Auto);
    RTTI_ENUM_OPTION(None);
    RTTI_ENUM_OPTION(ByAlpha);
    RTTI_ENUM_OPTION(NonBlack);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(ImageCompressionQuality);
    RTTI_ENUM_OPTION(Quick);
    RTTI_ENUM_OPTION(Normal);
    RTTI_ENUM_OPTION(Placebo);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(ImageAlphaMode);
    RTTI_ENUM_OPTION(NotPremultiplied);
    RTTI_ENUM_OPTION(AlreadyPremultiplied);
    RTTI_ENUM_OPTION(Premultiply);
    RTTI_ENUM_OPTION(RemoveAlpha);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_ENUM(ImageCompressionFormat);
    RTTI_ENUM_OPTION(Auto);
    RTTI_ENUM_OPTION(None);
    //RTTI_ENUM_OPTION(RGB565);
    //RTTI_ENUM_OPTION(RGBA4444);
    RTTI_ENUM_OPTION(BC1);
    RTTI_ENUM_OPTION(BC2);
    RTTI_ENUM_OPTION(BC3);
    RTTI_ENUM_OPTION(BC4);
    RTTI_ENUM_OPTION(BC5);
    RTTI_ENUM_OPTION(BC6H);
    RTTI_ENUM_OPTION(BC7);
RTTI_END_TYPE();

//--

RTTI_BEGIN_TYPE_CLASS(ImageCompressionSettings);
    RTTI_CATEGORY("Content");
    RTTI_PROPERTY(m_contentType).editable("Content type of the image, usually guessed by the file name postfix");
    RTTI_PROPERTY(m_contentColorSpace).editable("Color space of the image's content");
    RTTI_PROPERTY(m_contentAlphaMode).editable("Alpha mode of the image");
    RTTI_CATEGORY("Mipmapping");
    RTTI_PROPERTY(m_mipmapMode).editable("Mipmap generation mode");
    RTTI_CATEGORY("Compression");
    RTTI_PROPERTY(m_compressionMode).editable("Compression mode");
    RTTI_PROPERTY(m_compressionMasking).editable("Masking mode for determining pixels taking part in compression");
    RTTI_PROPERTY(m_compressionQuality).editable("Compression quality");
RTTI_END_TYPE();

ImageCompressionSettings::ImageCompressionSettings()
{}

//--

static image::PixelFormat GetImagePixelFormat(ImageFormat format)
{
    switch (format)
    {
    case ImageFormat::R16_INT:
    case ImageFormat::R16_UINT:
    case ImageFormat::R16_SNORM:
    case ImageFormat::R16_UNORM:
    case ImageFormat::RG16_INT:
    case ImageFormat::RG16_UINT:
    case ImageFormat::RG16_UNORM:
    case ImageFormat::RG16_SNORM:
    case ImageFormat::RGBA16_INT:
    case ImageFormat::RGBA16_UINT:
    case ImageFormat::RGBA16_UNORM:
    case ImageFormat::RGBA16_SNORM:
        return image::PixelFormat::Uint8_Norm;

    case ImageFormat::R16F:
    case ImageFormat::RG16F:
    case ImageFormat::RGBA16F:
        return image::PixelFormat::Float16_Raw;

    case ImageFormat::R32F:
    case ImageFormat::RG32F:
    case ImageFormat::RGBA32F:
        return image::PixelFormat::Float32_Raw;
    }

    return image::PixelFormat::Uint8_Norm;
}

static ImageFormat ChooseCompressedImageFormat(ImageCompressionFormat format, ImageContentColorSpace colorSpace)
{
    const bool srgb = (colorSpace == ImageContentColorSpace::SRGB);

    switch (format)
    {
        case ImageCompressionFormat::BC1:
            return srgb ? ImageFormat::BC1_SRGB : ImageFormat::BC1_UNORM;

        case ImageCompressionFormat::BC2:
            return srgb ? ImageFormat::BC2_SRGB : ImageFormat::BC2_UNORM;

        case ImageCompressionFormat::BC3:
            return srgb ? ImageFormat::BC3_SRGB : ImageFormat::BC3_UNORM;

        case ImageCompressionFormat::BC4:
            return ImageFormat::BC4_UNORM;

        case ImageCompressionFormat::BC5:
            return ImageFormat::BC5_UNORM;

        case ImageCompressionFormat::BC6H:
            if (colorSpace == ImageContentColorSpace::Linear)
                return ImageFormat::BC6_SIGNED;
            else
                return ImageFormat::BC6_UNSIGNED;

        case ImageCompressionFormat::BC7:
            if (colorSpace == ImageContentColorSpace::SRGB)
                return ImageFormat::BC7_SRGB;
            else
                return ImageFormat::BC7_UNORM;
    }

    return ImageFormat::UNKNOWN;
}

static ImageFormat ChooseUncompressedFormat(image::PixelFormat format, uint8_t channels, ImageContentColorSpace colorSpace, uint8_t& outRequiredChannelCount)
{
    outRequiredChannelCount = channels;

    const bool srgb = (colorSpace == ImageContentColorSpace::SRGB);

    switch (format)
    {
        case image::PixelFormat::Uint8_Norm:
        {
            if (srgb)
            {
                switch (channels)
                {
                case 1: outRequiredChannelCount = 3; return ImageFormat::SRGB8;
                case 2: outRequiredChannelCount = 3; return ImageFormat::SRGB8;
                case 3: outRequiredChannelCount = 3; return ImageFormat::SRGB8;
                case 4: outRequiredChannelCount = 4; return ImageFormat::SRGBA8;
                }
            }
            else
            {
                switch (channels)
                {
                case 1: return ImageFormat::R8_UNORM;
                case 2: return ImageFormat::RG8_UNORM;
                case 3: return ImageFormat::RGB8_UNORM;
                case 4: return ImageFormat::RGBA8_UNORM;
                }
            }
            break;
        }

        case image::PixelFormat::Uint16_Norm:
        {
            switch (channels)
            {
            case 1: return ImageFormat::R16_UNORM;
            case 2: return ImageFormat::RG16_UNORM;
            case 3: outRequiredChannelCount = 4; return ImageFormat::RGBA16_UNORM;
            case 4: return ImageFormat::RGBA16_UNORM;
            }
            break;
        }

        case image::PixelFormat::Float16_Raw:
        {
            switch (channels)
            {
            case 1: return ImageFormat::R16F;
            case 2: return ImageFormat::RG16F;
            case 3: outRequiredChannelCount = 4; return ImageFormat::RGBA16F;
            case 4: return ImageFormat::RGBA16F;
            }
            break;
        }

        case image::PixelFormat::Float32_Raw:
        {
            switch (channels)
            {
            case 1: return ImageFormat::R32F;
            case 2: return ImageFormat::RG32F;
            case 3: return ImageFormat::RGB32F;
            case 4: return ImageFormat::RGBA32F;
            }
            break;
        }
    }

    DEBUG_CHECK(!"Invalid format");
    return ImageFormat::UNKNOWN;
}

ImageCompressionFormat ChooseAutoCompressedFormat(image::PixelFormat format, uint8_t channels, ImageContentType type)
{
    if (format == image::PixelFormat::Float16_Raw || format == image::PixelFormat::Float32_Raw)
    {
        if (channels == 4)
            return ImageCompressionFormat::None;
        else
            return ImageCompressionFormat::BC6H;
    }

    if (type == ImageContentType::CombinedRoughnessSpecularity)
        if (channels > 2)
            return ImageCompressionFormat::BC3;
            
    if (type == ImageContentType::TangentNormalMap)
    {
        if (channels == 4)
            return ImageCompressionFormat::BC3;
        else
            return ImageCompressionFormat::BC5;
    }

    if (type == ImageContentType::Roughness || type == ImageContentType::Specularity)
        if (channels == 4)
            return ImageCompressionFormat::BC3;

    if (type == ImageContentType::AmbientOcclusion || type == ImageContentType::Metalness || type == ImageContentType::Mask 
        || type == ImageContentType::Roughness || type == ImageContentType::Specularity || type == ImageContentType::Bumpmap)
        return ImageCompressionFormat::BC4;

    if (channels == 1)
        return ImageCompressionFormat::BC4;
    else if (channels == 2)
        return ImageCompressionFormat::BC5;
    else if (channels == 4)
        return ImageCompressionFormat::BC3;
    else
        return ImageCompressionFormat::BC1;
}

ImageValidPixelsMaskingMode ChooseAutoMaskingMode(ImageContentType content, ImageContentColorSpace space)
{
    switch (content)
    {
        case ImageContentType::Generic:
        case ImageContentType::Albedo:
            return ImageValidPixelsMaskingMode::ByAlpha;

        case ImageContentType::TangentNormalMap:
        case ImageContentType::WorldNormalMap:
            return ImageValidPixelsMaskingMode::NonBlack;
    }

    return ImageValidPixelsMaskingMode::None;
}

ImageMipmapGenerationMode ChooseAutoMipMode(image::PixelFormat format, uint8_t channels, ImageContentType type)
{
    return ImageMipmapGenerationMode::BoxFilter;
}

ImageContentColorSpace ChooseAutoColorSpace(ImageContentType type, image::PixelFormat format)
{
    if (format == image::PixelFormat::Float16_Raw || format == image::PixelFormat::Float32_Raw)
        return ImageContentColorSpace::HDR;

    if (format == image::PixelFormat::Uint16_Norm)
        return ImageContentColorSpace::Linear;

    switch (type)
    {
        case ImageContentType::Specularity:
        case ImageContentType::Metalness:
        case ImageContentType::Roughness:
        case ImageContentType::Mask:
        case ImageContentType::Bumpmap:
        case ImageContentType::AmbientOcclusion:
            return ImageContentColorSpace::Linear;

        case ImageContentType::TangentNormalMap:
        case ImageContentType::WorldNormalMap:
            return ImageContentColorSpace::Normals;
    }

    return ImageContentColorSpace::SRGB;
}

ImageContentType ChooseAutoContentType(StringView suffix, image::PixelFormat format, uint32_t channels)
{
    if (!suffix.empty())
    {
        StringBuf txt = StringBuf(suffix).toLower();

        if (txt == "a" || txt == "d" || txt == "diff" || txt == "diffuse" || txt == "albedo" || txt == "color" || txt == "base_color" || txt == "basecolor")
            return ImageContentType::Albedo;
        if (txt == "ao")
            return ImageContentType::AmbientOcclusion;
        if (txt == "n" || txt == "norm" || txt == "normal" || txt == "ddna" || txt == "ns")
            return ImageContentType::TangentNormalMap;
        if (txt == "b" || txt == "bump")
            return ImageContentType::Bumpmap;
        if (txt == "s" || txt == "spec")
            return ImageContentType::Specularity;
        if (txt == "MetallicSmoothness")
            return ImageContentType::CombinedMetallicSmoothness;
        if (txt == "rs")
            return ImageContentType::CombinedRoughnessSpecularity;
        if (txt == "r" || txt == "rough" || txt == "roughness" || txt == "rf")
            return ImageContentType::Roughness;
        if (txt == "m" || txt == "metallic" || txt == "metalness")
            return ImageContentType::Metalness;
        if (txt == "mask")
            return ImageContentType::Generic;
        if (txt == "e" || txt == "emissive")
            return ImageContentType::Emissive;

        TRACE_WARNING("Unrecognized texture suffix '{}' no content group will be assigned", suffix);
    }

    return ImageContentType::Generic;
}

image::ColorSpace TranslateColorSpace(ImageContentColorSpace colorSpace)
{
    switch (colorSpace)
    {
    case ImageContentColorSpace::HDR: return image::ColorSpace::HDR;
    case ImageContentColorSpace::SRGB: return image::ColorSpace::SRGB;
    case ImageContentColorSpace::Normals: return image::ColorSpace::Normals;
    }

    return image::ColorSpace::Linear;
}

image::ImagePtr ChangeChannelCount(const image::ImageView& data, uint8_t targetChannelCount)
{
    auto ret = RefNew<image::Image>(data.format(), targetChannelCount, data.width(), data.height(), data.depth());
    if (!ret)
        return nullptr;

    image::ConvertChannels(data, ret->view());
    return ret;
}

//--

struct TempMip
{
    Buffer data;
    StaticTextureMip mip;

    void setupFromImage(const image::ImageView& view)
    {
        mip.compressed = false;
        mip.dataOffset = 0;
        mip.dataSize = 0;
        mip.width = view.width();
        mip.height = view.height();
        mip.depth = view.depth();
        mip.rowPitch = view.rowPitch();
        mip.slicePitch = view.slicePitch();
    }
};

//--

static RefPtr<ImageCompressedResult> AssembleFinalResult(const Array<TempMip>& mips)
{
    const auto textureMipAlignment = 256U;

    if (mips.empty())
        return nullptr;

    auto ret = RefNew<ImageCompressedResult>();
    ret->mips.reserve(mips.size());

    uint64_t totalDataSize = 0;
    for (uint32_t i = 0; i < mips.size(); ++i)
    {
        const auto& sourceMip = mips[i];
        auto& destMip = ret->mips.emplaceBack(sourceMip.mip);
        destMip.dataOffset = Align<uint32_t>(totalDataSize, textureMipAlignment);
        totalDataSize = destMip.dataOffset + destMip.dataSize;
    }

    ret->data = Buffer::Create(POOL_IMAGE, totalDataSize, 16);
    if (!ret->data)
        return nullptr;

    for (uint32_t i = 0; i < mips.size(); ++i)
    {
        const auto& sourceMip = mips[i];
        const auto& destMip = ret->mips[i];

        memcpy(ret->data.data() + destMip.dataOffset, sourceMip.data.data(), destMip.dataSize);
    }

    return ret;
}

//--

uint32_t CalcCompressedImageDataSize(const image::ImageView& data, ImageCompressionFormat format)
{
    if (format == ImageCompressionFormat::None || format == ImageCompressionFormat::Auto)
        return data.pixelPitch() * data.width() * data.height() * data.depth();

    DEBUG_CHECK_EX(data.depth() == 1, "3D textures cannot be compressed like that");

    const auto numBlocksX = std::max<uint32_t>(1, (data.width() + 3) / 4);
    const auto numBlocksY = std::max<uint32_t>(1, (data.height() + 3) / 4);

    uint32_t bytesPerBlock = 16;
    if (format == ImageCompressionFormat::BC1 || format == ImageCompressionFormat::BC4)
        bytesPerBlock = 8;

    return numBlocksX * numBlocksY * bytesPerBlock;
}

struct Half
{
    uint16_t val;
};

INLINE void ConvertChannelValue(uint8_t& to, uint8_t from) { to = from; }
INLINE void ConvertChannelValue(uint8_t& to, uint16_t from) { to = (uint8_t)(from >> 8); }
INLINE void ConvertChannelValue(uint8_t& to, Half from) { to = (uint8_t)std::clamp<float>(Float16Helper::Decompress(from.val) * 255.0f, 0.0f, 255.0f); }
INLINE void ConvertChannelValue(uint8_t& to, float from) { to = (uint8_t)std::clamp<float>(from * 255.0f, 0.0f, 255.0f); }
    
INLINE void ConvertChannelValue(uint16_t& to, uint8_t from) { to = ((uint16_t)from) | (((uint16_t)from) << 8); }
INLINE void ConvertChannelValue(uint16_t& to, uint16_t from) { to = from; }
INLINE void ConvertChannelValue(uint16_t& to, Half from) { to = (uint16_t)std::clamp<float>(Float16Helper::Decompress(from.val) * 65535, 0.0f, 65535); }
INLINE void ConvertChannelValue(uint16_t& to, float from) { to = (uint16_t)std::clamp<float>(from * 65535.0f, 0.0f, 65535.0f); }
    
INLINE void ConvertChannelValue(Half& to, uint8_t from) { to.val = Float16Helper::Compress(from / 255.0f); }
INLINE void ConvertChannelValue(Half& to, uint16_t from) { to.val = Float16Helper::Compress(from / 65535.0f); }
INLINE void ConvertChannelValue(Half& to, Half from) { to = from; }
INLINE void ConvertChannelValue(Half& to, float from) { to.val = Float16Helper::Compress(from); }
    
INLINE void ConvertChannelValue(float& to, uint8_t from) { to = from / 255.0f; }
INLINE void ConvertChannelValue(float& to, uint16_t from) { to = from / 65535.0f; }
INLINE void ConvertChannelValue(float& to, Half from) { to = Float16Helper::Decompress(from.val); }
INLINE void ConvertChannelValue(float& to, float from) { to = from; }

INLINE void ConvertChannelValue(double& to, uint8_t from) { to = from; }
INLINE void ConvertChannelValue(double& to, uint16_t from) { to = from / 256.0; }
INLINE void ConvertChannelValue(double& to, Half from) { to = Float16Helper::Decompress(from.val); }
INLINE void ConvertChannelValue(double& to, float from) { to = from; }

static const auto ONE_HALF = Float16Helper::Compress(1.0f);

INLINE void FillAlphaChannelToOne(uint8_t& to) { to = 255; }
INLINE void FillAlphaChannelToOne(uint16_t& to) { to = 65535; }
INLINE void FillAlphaChannelToOne(Half& to) { to.val = ONE_HALF; }
INLINE void FillAlphaChannelToOne(float& to) { to = 1.0f; }
INLINE void FillAlphaChannelToOne(double& to) { to = 255.0; }

template< uint32_t N, typename ST, typename DT >
void CopyBlockPixels(const image::ImageView& data, uint32_t& outMask, DT* writePtr)
{
    for (image::ImageViewRowIterator y(data); y; ++y)
    {
        auto rowPtr = writePtr;
        for (image::ImageViewPixelIterator x(y); x; ++x, rowPtr += 4)
        {
            auto* srcData = (const ST*)x.data();
            switch (N)
            {
            case 4: ConvertChannelValue(rowPtr[3], srcData[3]);
            case 3: ConvertChannelValue(rowPtr[2], srcData[2]);
            case 2: ConvertChannelValue(rowPtr[1], srcData[1]);
            case 1: ConvertChannelValue(rowPtr[0], srcData[0]);
            }

            if (N == 3)
                FillAlphaChannelToOne(rowPtr[3]);

            outMask |= (1 << (4 * y.pos() + x.pos()));
        }
        writePtr += 16;
    }
}

template< typename DT >
void CopyBlockPixels(const image::ImageView& data, uint32_t& outMask, DT* targetMemory)
{
    DEBUG_CHECK_EX(data.width() <= 4, "Invalid block size");
    DEBUG_CHECK_EX(data.height() <= 4, "Invalid block size");

    switch (data.format())
    {
        case image::PixelFormat::Uint8_Norm:
        {
            switch (data.channels())
            {
                case 1: CopyBlockPixels<1, uint8_t, DT>(data, outMask, targetMemory); break;
                case 2: CopyBlockPixels<2, uint8_t, DT>(data, outMask, targetMemory); break;
                case 3: CopyBlockPixels<3, uint8_t, DT>(data, outMask, targetMemory); break;
                case 4: CopyBlockPixels<4, uint8_t, DT>(data, outMask, targetMemory); break;
            }
            break;
        }

        case image::PixelFormat::Uint16_Norm:
        {
            switch (data.channels())
            {
            case 1: CopyBlockPixels<1, uint16_t, DT>(data, outMask, targetMemory); break;
            case 2: CopyBlockPixels<2, uint16_t, DT>(data, outMask, targetMemory); break;
            case 3: CopyBlockPixels<3, uint16_t, DT>(data, outMask, targetMemory); break;
            case 4: CopyBlockPixels<4, uint16_t, DT>(data, outMask, targetMemory); break;
            }
            break;
        }

        case image::PixelFormat::Float16_Raw:
        {
            switch (data.channels())
            {
            case 1: CopyBlockPixels<1, Half, DT>(data, outMask, targetMemory); break;
            case 2: CopyBlockPixels<2, Half, DT>(data, outMask, targetMemory); break;
            case 3: CopyBlockPixels<3, Half, DT>(data, outMask, targetMemory); break;
            case 4: CopyBlockPixels<4, Half, DT>(data, outMask, targetMemory); break;
            }
            break;
        }

        case image::PixelFormat::Float32_Raw:
        {
            switch (data.channels())
            {
            case 1: CopyBlockPixels<1, float, DT>(data, outMask, targetMemory); break;
            case 2: CopyBlockPixels<2, float, DT>(data, outMask, targetMemory); break;
            case 3: CopyBlockPixels<3, float, DT>(data, outMask, targetMemory); break;
            case 4: CopyBlockPixels<4, float, DT>(data, outMask, targetMemory); break;
            }
            break;
        }

        default:
            DEBUG_CHECK(!"Unknown type");
    }
}

static Mutex GCompressionTableLock;

class BlockCompressor
{
public:
    BlockCompressor(uint32_t mipIndex, uint32_t totalMips, const image::ImageView& fullData, ImageFormat targetFormat, uint32_t squishFlags, ImageValidPixelsMaskingMode masking, uint8_t* targetMemory, IProgressTracker& progress)
        : m_progress(progress)
        , m_squishFlags(squishFlags)
        , m_masking(masking)
        , m_targetMemory(targetMemory)
        , m_targetFormat(targetFormat)
        , m_fullData(fullData)
        , m_mipIndex(mipIndex)
        , m_totalMips(totalMips)
    {
        m_blockWidth = std::max<uint32_t>(1, (fullData.width() + 3) / 4);
        m_blockHeight = std::max<uint32_t>(1, (fullData.height() + 3) / 4);

        m_megaBlockSideSize = std::clamp<uint32_t>(cvTextureCompressionBlockSize.get(), 1, 16);
        m_megaBlockWidth = (m_blockWidth + m_megaBlockSideSize - 1) / m_megaBlockSideSize;
        m_megaBlockHeight = (m_blockHeight + m_megaBlockSideSize - 1) / m_megaBlockSideSize;
        m_megaBlocksTotalCount = m_megaBlockWidth * m_megaBlockHeight;

        m_blockDataSize = 16;
        if ((m_squishFlags & squish::kBc4) || (m_squishFlags & squish::kDxt1))
            m_blockDataSize = 8;

        auto maxBlocks = m_megaBlockWidth * m_megaBlockHeight;
        auto maxJobs = std::max<uint32_t>(1, std::min<uint32_t>(cvMaxTextureCompressionThreads.get(), maxBlocks));

        if (cvMaxTextureCompressionThreads.get() <= 0)
            maxJobs = std::max<int>(1, Fibers::GetInstance().workerThreadCount() + cvMaxTextureCompressionThreads.get());

        if (targetFormat == ImageFormat::BC7_SRGB || targetFormat == ImageFormat::BC7_UNORM)
        {
            auto lock = CreateLock(GCompressionTableLock);
            Quant_Init();
            init_ramps();
        }

        m_finishCounter = Fibers::GetInstance().createCounter("ImageCompression", maxJobs);
        for (uint32_t i = 0; i < maxJobs; ++i)
            startJobLoop();
    }

    inline bool canceled() const
    {
        return m_canceled;
    }

    void wait()
    {
        Fibers::GetInstance().waitForCounterAndRelease(m_finishCounter);
    }

protected:
    bool popMegaBlock(uint32_t& outX, uint32_t& outY)
    {
        auto lock = CreateLock(m_blockCurrentLock);

        if (m_progress.checkCancelation())
        {
            m_canceled = true;
            return false; // canceled
        }

        if (m_megaBlockCurrentX == m_megaBlockWidth && m_megaBlockCurrentY == m_megaBlockHeight)
            return false;

        outX = m_megaBlockCurrentX;
        outY = m_megaBlockCurrentY;

        m_megaBlockCurrentX += 1;
        if (m_megaBlockCurrentX == m_megaBlockWidth)
        {
            m_megaBlockCurrentY += 1;

            if (m_megaBlockCurrentY < m_megaBlockHeight)
                m_megaBlockCurrentX = 0;
        }

        return true;
    }

    void startJob()
    {
        uint32_t x, y;
        if (popMegaBlock(x, y))
        {
            RunChildFiber("ImageBlockCompression") << [this, x, y](FIBER_FUNC)
            {
                processMegaBlock(x, y);
                startJob();
            };
        }
        else
        {
            TRACE_INFO("No more blocks to process");
            Fibers::GetInstance().signalCounter(m_finishCounter);
        }
    }

    void startJobLoop()
    {
        RunChildFiber("ImageBlockCompression") << [this](FIBER_FUNC)
        {
            uint32_t x, y;
            while (popMegaBlock(x, y))
            {
                processMegaBlock(x, y);
                Fibers::GetInstance().yield();
            }

            Fibers::GetInstance().signalCounter(m_finishCounter);
        };
    }

    void processMegaBlock(uint32_t x, uint32_t y)
    {
        const auto ox = x * m_megaBlockSideSize;
        const auto oy = y * m_megaBlockSideSize;
        for (uint32_t by=0; by<m_megaBlockSideSize; ++by)
            for (uint32_t bx = 0; bx < m_megaBlockSideSize; ++bx)
                processBlock(ox + bx, oy + by);

        auto blockIndex = ++m_processedMegaBlocksCounter;
        m_progress.reportProgress(blockIndex, m_megaBlocksTotalCount, TempString("Compressing mip {} ({})", m_mipIndex, m_targetFormat));
    }

    void processBlock(uint32_t x, uint32_t y)
    {
        auto subImageX = x * 4;
        auto subImageY = y * 4;
        auto subImageW = std::min<uint32_t>(4, m_fullData.width() - subImageX);
        auto subImageH = std::min<uint32_t>(4, m_fullData.height() - subImageY);

        if (subImageX >= m_fullData.width() || subImageY >= m_fullData.height())
            return;

        auto subView = m_fullData.subView(subImageX, subImageY, subImageW, subImageH);

        auto* targetBlockData = m_targetMemory + (x + y * m_blockWidth) * m_blockDataSize;

        if (m_targetFormat == ImageFormat::BC6_UNSIGNED || m_targetFormat == ImageFormat::BC6_SIGNED)
        {
            float sourceRgba[16][4];
            memset(sourceRgba, 0, sizeof(sourceRgba));

            uint32_t sourcePixelMask = 0;
            CopyBlockPixels<float>(subView, sourcePixelMask, &sourceRgba[0][0]);

            CMP_BC6H_BLOCK_PARAMETERS params;
            params.bIsSigned = (m_targetFormat == ImageFormat::BC6_SIGNED);

            BC6HBlockEncoder encoder(params);
            encoder.CompressBlock(sourceRgba, targetBlockData);
        }
        else if (m_targetFormat == ImageFormat::BC7_SRGB || m_targetFormat == ImageFormat::BC7_UNORM)
        {
            double sourceRgba[16][4];
            memset(sourceRgba, 0, sizeof(sourceRgba));

            uint32_t sourcePixelMask = 0;
            CopyBlockPixels<double>(subView, sourcePixelMask, &sourceRgba[0][0]);

            unsigned int validModeMask = 0xFFFFFFFFU;
            bool needsAlpha = (subView.channels() == 4);

            BC7BlockEncoder encoder(validModeMask, needsAlpha, 0.6, true, true);
            encoder.CompressBlock(sourceRgba, targetBlockData);
        }
        else
        {
            uint8_t sourceRgba[16 * 4];
            memset(sourceRgba, 0xFF, sizeof(sourceRgba));

            uint32_t sourcePixelMask = 0;
            CopyBlockPixels<uint8_t>(subView, sourcePixelMask, sourceRgba);

            auto squishFlags = m_squishFlags;
            if (m_masking == ImageValidPixelsMaskingMode::ByAlpha)
                squishFlags |= squish::kWeightColourByAlpha;

            // TODO: other masking modes

            squish::CompressMasked(sourceRgba, sourcePixelMask, targetBlockData, squishFlags);
        }
    }

private:
    bool m_canceled = false;

    IProgressTracker& m_progress;
    uint32_t m_blockWidth;
    uint32_t m_blockHeight;

    image::ImageView m_fullData;

    uint32_t m_squishFlags = 0;
    ImageValidPixelsMaskingMode m_masking = ImageValidPixelsMaskingMode::Auto;
    uint8_t* m_targetMemory = nullptr;
    ImageFormat m_targetFormat;

    uint32_t m_megaBlockSideSize = 0;
    uint32_t m_megaBlockWidth = 0;
    uint32_t m_megaBlockHeight = 0;
    uint32_t m_megaBlockCurrentX = 0;
    uint32_t m_megaBlockCurrentY = 0;
    uint32_t m_megaBlocksTotalCount = 0;
    SpinLock m_blockCurrentLock;

    std::atomic<uint32_t> m_processedMegaBlocksCounter = 0;
    std::atomic<uint32_t> m_activeMegaBlocks = 0;

    uint32_t m_blockDataSize = 0;

    uint32_t m_mipIndex = 0;
    uint32_t m_totalMips = 0;

    fibers::WaitCounter m_finishCounter;
};
     
//--

static uint32_t CalcMipCount(const image::ImageView& data)
{
    uint32_t w = data.width();
    uint32_t h = data.height();
    uint32_t d = data.depth();

    uint32_t count = 1;
    while (w > 1 || h > 1 || d > 1)
    {
        count += 1;
        w = std::max<uint32_t>(w / 2, 1);
        h = std::max<uint32_t>(h / 2, 1);
        d = std::max<uint32_t>(d / 2, 1);
    }

    return count;
}

//--

static image::ImageView DownsampleImage(const image::ImageView& sourceView, image::ImagePtr& tempImage, ImageValidPixelsMaskingMode maskMode, ImageMipmapGenerationMode mode, ImageContentColorSpace space, bool hasPremultipliedAlpha)
{
    const auto mipW = std::max<uint32_t>(sourceView.width() / 2, 1);
    const auto mipH = std::max<uint32_t>(sourceView.height() / 2, 1);
    const auto mipD = std::max<uint32_t>(sourceView.depth() / 2, 1);

    image::ColorSpace downsampleColorSpace = image::ColorSpace::Linear;
    if (space == ImageContentColorSpace::SRGB)
        downsampleColorSpace = image::ColorSpace::SRGB;
    else if (space == ImageContentColorSpace::HDR)
        downsampleColorSpace = image::ColorSpace::HDR;

    image::DownsampleMode downsampleMode = image::DownsampleMode::Average;
    if (space == ImageContentColorSpace::SRGB || space == ImageContentColorSpace::Linear || space == ImageContentColorSpace::HDR)
    {
        if (maskMode == ImageValidPixelsMaskingMode::ByAlpha)
            if (hasPremultipliedAlpha)
                downsampleMode = image::DownsampleMode::Average;
            else 
                downsampleMode = image::DownsampleMode::AverageWithAlphaWeight;
    }

    // create a compatible image with half the size
    if (auto mipImage = RefNew<image::Image>(sourceView.format(), sourceView.channels(), mipW, mipH, mipD))
    {
        // downsample the image
        image::Downsample(sourceView, mipImage->view(), downsampleMode, downsampleColorSpace);

        // return new data set
        tempImage = mipImage;
        return mipImage->view();
    }

    return image::ImageView();
}

template< typename T >
static bool CheckIfImageHasAlphaData(const image::ImageView& data, T defaultAlphaValue)
{
    for (image::ImageViewSliceIterator z(data); z; ++z)
    {
        for (image::ImageViewRowIterator y(z); y; ++y)
        {
            for (image::ImageViewPixelIterator x(y); x; ++x)
            {
                const auto* data = ((const T*)x.data());
                if (data[3] != defaultAlphaValue)
                    return true;
            }
        }
    }

    return false;
}

static bool CheckIfImageHasAlphaData(const image::ImageView& data)
{
    if (data.channels() != 4)
        return false;

    static const uint16_t HALF_ONE = Float16Helper::Compress(1.0f);

    switch (data.format())
    {
        case image::PixelFormat::Uint8_Norm: return CheckIfImageHasAlphaData<uint8_t>(data, 255);
        case image::PixelFormat::Uint16_Norm: return CheckIfImageHasAlphaData<uint16_t>(data, 65535);
        case image::PixelFormat::Float16_Raw: return CheckIfImageHasAlphaData<uint16_t>(data, HALF_ONE);
        case image::PixelFormat::Float32_Raw: return CheckIfImageHasAlphaData<float>(data, 1.0f);
    }

    return false;
}

//--

RefPtr<ImageCompressedResult> CompressImage(const image::ImageView& data, const ImageCompressionSettings& settings, IProgressTracker& progress)
{
    if (data.empty())
    {
        TRACE_WARNING("Trying to compress empty image");
        return nullptr;
    }

    if (data.width() > cvMaxTextureWidth.get() || data.height() > cvMaxTextureHeight.get() || data.depth() > cvMaxTextureDepth.get())
    {
        TRACE_ERROR("Source image dimensions [{}x{}x{}] are larger than maximum supported texture size [{}x{}x{}]",
            data.width(), data.height(), data.depth(), cvMaxTextureWidth.get(), cvMaxTextureHeight.get(), cvMaxTextureDepth.get());
        return nullptr;
    }

    const auto numPixels = data.width() * data.height() * data.depth();
    const auto totalPixelDataSize = (numPixels * data.pixelPitch()) >> 20;
    if (totalPixelDataSize > cvMaxTextureDataSizeMB.get())
    {
        TRACE_ERROR("Source image dimensions [{}x{}x{}] consume more memory ({} MB) than allowed maximum ({} MB)",
            data.width(), data.height(), data.depth(), totalPixelDataSize, cvMaxTextureDataSizeMB.get());
        return nullptr;
    }

    // determine content type
    auto contentType = settings.m_contentType;
    if (contentType == ImageContentType::Auto)
        contentType = ChooseAutoContentType(settings.m_suffix, data.format(), data.channels());

    // determine color space
    auto colorSpace = settings.m_contentColorSpace;
    if (colorSpace == ImageContentColorSpace::Auto) 
        colorSpace = ChooseAutoColorSpace(contentType, data.format());

    // drop alpha channel
    uint8_t requiredChannelCount = data.channels();
    if (requiredChannelCount == 4 && (settings.m_contentAlphaMode == ImageAlphaMode::RemoveAlpha || !CheckIfImageHasAlphaData(data)))
        requiredChannelCount = 3;

    auto uncomressedImageView = data;
    auto uncompressedFormat = ChooseUncompressedFormat(data.format(), requiredChannelCount, colorSpace, requiredChannelCount);

    image::ImagePtr tempImage;
    if (requiredChannelCount != data.channels())
    {
        TRACE_WARNING("Compressing this image requires changing channel count {} -> {}", data.channels(), requiredChannelCount);
        tempImage = ChangeChannelCount(data, requiredChannelCount);
        if (!tempImage)
        {
            TRACE_ERROR("Source image dimensions [{}x{}x{}] cannot be converted to new channel count", data.width(), data.height(), data.depth());
            return nullptr;
        }

        uncomressedImageView = tempImage->view();
    }

    // premultiply the alpha channel data
    bool hasPremultipliedAlpha = false;
    if (uncomressedImageView.channels() == 4)
    {
        if (settings.m_contentAlphaMode == ImageAlphaMode::Premultiply)
        {
            if (!tempImage)
            {
                tempImage = RefNew<image::Image>(data.format(), 4, data.width(), data.height(), data.depth());
                image::Copy(uncomressedImageView, tempImage->view());
                uncomressedImageView = tempImage->view();
            }

            image::PremultiplyAlpha(uncomressedImageView);
        }
        else if (settings.m_contentAlphaMode == ImageAlphaMode::AlreadyPremultiplied)
        {
            hasPremultipliedAlpha = true;
        }
    }

    // choose best compressed format
    auto compression = settings.m_compressionMode;
    if (compression == ImageCompressionFormat::Auto)
        compression = ChooseAutoCompressedFormat(uncomressedImageView.format(), uncomressedImageView.channels(), contentType);
    /*if (!allowCompression)
        compression = ImageCompressionFormat::None;*/

    // masking
    auto masking = settings.m_compressionMasking;
    if (masking == ImageValidPixelsMaskingMode::Auto)
        masking = ChooseAutoMaskingMode(contentType, colorSpace);

    // choose best mipmap mode
    auto mipMode = settings.m_mipmapMode;
    if (mipMode == ImageMipmapGenerationMode::Auto)
        mipMode = ChooseAutoMipMode(uncomressedImageView.format(), uncomressedImageView.channels(), contentType);

    // adjust compressed format
    auto compressedFormat = (compression == ImageCompressionFormat::None) ? uncompressedFormat : ChooseCompressedImageFormat(compression, colorSpace);
            
    // calculate mip count
    auto mipCount = (mipMode == ImageMipmapGenerationMode::None) ? 1 : CalcMipCount(data);

    // determine compression flags
    uint32_t squishFlags = 0;
    if (settings.m_compressionQuality == ImageCompressionQuality::Placebo)
        squishFlags |= squish::kColourIterativeClusterFit;
    else if (settings.m_compressionQuality == ImageCompressionQuality::Quick)
        squishFlags |= squish::kColourRangeFit;
    else if (settings.m_compressionQuality == ImageCompressionQuality::Normal)
        squishFlags |= squish::kColourClusterFit;

    if (compression == ImageCompressionFormat::BC1)
        squishFlags |= squish::kDxt1;
    else if (compression == ImageCompressionFormat::BC2)
        squishFlags |= squish::kDxt3;
    else if (compression == ImageCompressionFormat::BC3)
        squishFlags |= squish::kDxt5;
    else if (compression == ImageCompressionFormat::BC4)
        squishFlags |= squish::kBc4;
    else if (compression == ImageCompressionFormat::BC5)
        squishFlags |= squish::kBc5;


    // create mipmap chain
    Array<TempMip> mips;
    mips.reserve(mipCount);
    for (uint32_t i = 0; i < mipCount; ++i)
    {
        auto& mip = mips.emplaceBack();
        mip.setupFromImage(uncomressedImageView);

        // check for task cancellation
        if (progress.checkCancelation())
            return nullptr;

        // compress the data or copy uncompressed data
        if (compression == ImageCompressionFormat::None)
        {
            mip.data = uncomressedImageView.toBuffer();
            if (!mip.data)
            {
                TRACE_ERROR("Out of memory when converting image data [{}x{}x{}] to buffer", uncomressedImageView.width(), uncomressedImageView.height(), uncomressedImageView.depth());
                return nullptr;
            }
        }
        else
        {
            const auto dataSize = CalcCompressedImageDataSize(uncomressedImageView, compression);
            mip.data = Buffer::Create(POOL_IMAGE, dataSize, 16);
            if (!mip.data)
            {
                TRACE_ERROR("Out of memory when converting image data [{}x{}x{}] to buffer", uncomressedImageView.width(), uncomressedImageView.height(), uncomressedImageView.depth());
                return nullptr;
            }
                 
            BlockCompressor compressor(i, mipCount, uncomressedImageView, compressedFormat, squishFlags, masking, mip.data.data(), progress);
            compressor.wait(); // TODO: we can process multiple mipmaps at the same time easily
        }

        // update mip
        mip.mip.dataSize = mip.data.size();

        // downsample the image
        progress.reportProgress(TempString("Downsampling to mip {}", i + 1));
        uncomressedImageView = DownsampleImage(uncomressedImageView, tempImage, masking, mipMode, colorSpace, hasPremultipliedAlpha);
        if (uncomressedImageView.empty())
        {
            TRACE_ERROR("Out of memory when downsampling image data [{}x{}x{}] to buffer", uncomressedImageView.width(), uncomressedImageView.height(), uncomressedImageView.depth());
            return nullptr;
        }
    }

    //--

    // final assemble
    progress.reportProgress("Assembling final output data");
    if (progress.checkCancelation())
        return nullptr;
            
    // create final result
    auto ret = AssembleFinalResult(mips);
    if (!ret)
    {
        TRACE_ERROR("Source image dimensions [{}x{}x{}] cannot be assembled into one buffer (OOM?)", data.width(), data.height(), data.depth());
        return nullptr;
    }

    // setup texture props
    ret->info.width = data.width();
    ret->info.height = data.height();
    ret->info.depth = data.depth();
    ret->info.format = compressedFormat;
    ret->info.mips = mips.size();
    ret->info.type = ImageViewType::View2D;
    ret->info.compressed = (compressedFormat != uncompressedFormat);
    ret->info.colorSpace = TranslateColorSpace(colorSpace);
    ret->info.premultipliedAlpha = hasPremultipliedAlpha;
    ret->info.slices = 1;
    return ret;
}

//--

#if 0
gpu::ImageObjectPtr ImageCompressedResult::createPreviewTexture() const
{
    gpu::ImageCreationInfo creationInfo;
    creationInfo.width = info.width;
    creationInfo.height = info.height;
    creationInfo.depth = info.depth;
    creationInfo.numSlices = 1;
    creationInfo.numMips = mips.size();
    creationInfo.view = ImageViewType::View2D;
    creationInfo.format = info.format;
    creationInfo.allowShaderReads = true;
    creationInfo.label = "PreviewTexture";

	//SourceDataProviderBuffer sourceData;
    /*InplaceArray<, 128> ;
    sourceData.resize(creationInfo.numMips * creationInfo.numSlices);

    auto* writePtr = sourceData.typedData();
    for (uint32_t i = 0; i < creationInfo.numSlices; ++i)
    {
        for (uint32_t j = 0; j < creationInfo.numMips; ++j, ++writePtr)
        {
            auto sourceMipIndex = (i * creationInfo.numMips) + j;
            if (sourceMipIndex < mips.size())
            {
                const auto& sourceMip = mips[sourceMipIndex];
                DEBUG_CHECK_EX(!sourceMip.streamed, "Streaming not yet supported");

                writePtr->data = data;
                writePtr->offset = sourceMip.dataOffset;
                writePtr->size = sourceMip.dataSize;
            }
        }
    }*/

    auto device = GetService<gpu::DeviceService>()->device();
	return device->createImage(creationInfo);// sourceData.typedData());
}
#endif

//--

END_BOOMER_NAMESPACE_EX(assets)
