/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "renderingTexture.h"

BEGIN_BOOMER_NAMESPACE(rendering)

//---

RTTI_BEGIN_TYPE_ENUM(TextureFilteringMode);
    RTTI_ENUM_OPTION(Point);
    RTTI_ENUM_OPTION(Bilinear);
    RTTI_ENUM_OPTION(Trilinear);
    RTTI_ENUM_OPTION(Aniso);
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_CLASS(TextureInfo);
    RTTI_PROPERTY(colorSpace);
    RTTI_PROPERTY(type);
    RTTI_PROPERTY(format);
    RTTI_PROPERTY(width);
    RTTI_PROPERTY(height);
    RTTI_PROPERTY(depth);
    RTTI_PROPERTY(slices);
    RTTI_PROPERTY(mips);
    RTTI_PROPERTY(filterMode);
    RTTI_PROPERTY(filterMaxAniso);
    RTTI_PROPERTY(compressed);
    RTTI_PROPERTY(premultipliedAlpha);
    //RTTI_PROPERTY(streamed);
RTTI_END_TYPE();

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ITexture);
    RTTI_PROPERTY(m_info);
RTTI_END_TYPE();

ITexture::ITexture(const TextureInfo& info)
    : m_info(info)
{
}

ITexture::~ITexture()
{
}

//---

#if 0

TRuntimeObjectPtr<Texture> Texture::CreateFromBlob(const TextureDataPtr& blob, const base::StringBuf& debugLabel)
{
    auto ret = CreateRuntimePtr<Texture>();
    ret->sourceData = blob;
    ret->m_debugLabel = debugLabel;
    ret->m_textureGroup = blob->textureGroupName();
    ret->initialize();
    return ret;
}

TRuntimeObjectPtr<Texture> Texture::CreateFromData(const ImageCreationInfo& setup, const SourceData* sourceData, const base::StringBuf& debugLabel)
{
    auto ret = CreateRuntimePtr<Texture>();

    auto numSlices = setup.m_numMips * setup.m_numSlices;
    ret->m_debugLabel = debugLabel;
    ret->m_rawImageInfo = setup;

    if (sourceData)
    {
        ret->m_rawSourceData.resize(numSlices);

        for (uint32_t i = 0; i < numSlices; ++i)
        {
            ret->m_rawSourceData[i].size = sourceData[i].size;
            ret->m_rawSourceData[i].offset = sourceData[i].offset;
            ret->m_rawSourceData[i].data = sourceData[i].data; // COPY?
        }
    }

    ret->initialize();
    return ret;
}

TRuntimeObjectPtr<Texture> Texture::CreateFromUserData(IDevice* drv, TextureType type, const base::image::ImagePtr* images, uint8_t numMips, uint16_t numSlices)
{
    // we need to have at least one mip and slice
    uint32_t numTotalImages = numMips * numSlices;
    if (0 == numTotalImages)
    {
        TRACE_ERROR("No images to create user texture from");
        return nullptr;
    }

    // get image params
    if (!images[0])
    {
        TRACE_ERROR("Invalid base image");
        return nullptr;
    }
    auto& baseImage = *images[0];

    // setup image creation info
    ImageCreationInfo info;
    info.type = ImageType::Static;
    info.format = TranslateImageFormat(baseImage.pixelFormat(), baseImage.size().m_channels);
    info.m_dim = TranslateImageDimensions(type);
    info.m_numSlices = numSlices;
    info.m_numMips = numMips;
    info.width = baseImage.size().width;
    info.height = baseImage.size().height;
    info.m_depth = baseImage.size().m_depth;

    // invalid format
    if (info.format == ImageFormat::UNKNOWN)
    {
        TRACE_ERROR("Invalid image format for source texture");
        return nullptr;
    }

    // cube flag
    if (type == TextureType::TypeCube || type == TextureType::TypeCubeArray)
        info.m_cube = 1;

    // setup source data
    base::Array<SourceData> sourceMips;
    base::Array<base::image::ImagePtr> sourceImages;
    sourceMips.reserve(numTotalImages);
    sourceImages.reserve(numTotalImages);
    uint32_t totalMemorySize = 0;
    uint32_t imageIndex = 0;
    for (uint32_t slice=0; slice<numSlices; ++slice)
    {
        for (uint32_t mip=0; mip<numMips; ++mip, ++imageIndex)
        {
            auto mipWidth = std::max<uint32_t>(1, info.width >> mip);
            auto mipHeight = std::max<uint32_t>(1, info.height >> mip);
            auto mipDepth = std::max<uint32_t>(1, info.m_depth >> mip);

            auto &sourceImage = images[imageIndex];
            if (!sourceImage)
            {
                TRACE_ERROR("Source image for slice {}, mip {} is empty", slice, mip);
                return nullptr;
            }

            // validate size
            if (sourceImage->size().width != mipWidth || sourceImage->size().height != mipHeight || sourceImage->size().m_depth != mipDepth)
            {
                TRACE_ERROR("Source image for slice {}, mip {} has invalid size [{}x{}x{}] instead of [{}x{}x{}]",
                        slice, mip, sourceImage->size().width, sourceImage->size().height, sourceImage->size().m_depth, mipWidth, mipHeight, mipDepth);
                return nullptr;
            }

            // conform to format
            auto convertedImage = base::image::ConvertPixelFormat(sourceImage, baseImage.pixelFormat());
            convertedImage = base::image::ConvertChannels(sourceImage, baseImage.size().m_channels, nullptr, base::Vector4(1, 1, 1, 1));

            // keep image around
            sourceImages.emplaceBack(sourceImage);

            // set data
            auto &sourceMip = sourceMips.emplaceBack();
            sourceMip.size = convertedImage->layout().m_layerPitch * convertedImage->size().m_depth;
            sourceMip.data = base::Buffer::Create(POOL_TEMP, sourceMip.size, 1, convertedImage->pixelBuffer());
            sourceMip.m_rowPitch = convertedImage->layout().m_rowPitch;
            sourceMip.m_slicePitch = convertedImage->layout().m_layerPitch;
            totalMemorySize += sourceMip.size;
        }
    }

    // create the image
    auto imageView = drv->createImage(info, sourceMips.typedData());
    if (imageView.empty())
        return nullptr;

    // create wrapper
    auto ret = new Texture;
    ret->m_memorySize = totalMemorySize;
    ret->m_mainView = imageView;
    return TRuntimeObjectPtr<Texture>(ret, false);
}

void Texture::calcMemoryUsage(uint32_t& outSystemMemory, uint32_t& outGraphicsMemory) const
{
    outSystemMemory += sizeof(Texture);

    if (m_mainView)
        outGraphicsMemory = m_mainView.calcMemorySize();
}

void Texture::describe(base::IFormatStream& f) const
{
    f.appendf("{}x{}x{}, {}, {}",
            m_mainView.width(), m_mainView.height(), m_mainView.depth(),
            base::reflection::GetEnumValueName(m_mainView.format()),
            m_textureGroup);
}

void Texture::releaseDriverRelatedData(IDevice* driver)
{
    // reset view
    m_mainView = ImageView::DefaultGrayLinear();

    // base cleanup
    TBaseClass::releaseDriverRelatedData(driver);
}

bool Texture::recreateDriverRelatedData(IDevice* driver)
{
    // base
    if (!TBaseClass::recreateDriverRelatedData(driver))
        return false;

    // raw data
    if (m_rawImageInfo.format != ImageFormat::UNKNOWN)
    {
        // create the image
        auto imageView = driver->createImage(m_rawImageInfo, m_rawSourceData.typedData());
        if (imageView.empty())
            return false;

        // create the sampler
        m_mainView = imageView.createSampledView(Sampler::DefaultBilinear());
        return true;
    }
    // blob
    else if (sourceData)
    {
        // setup image creation info
        ImageCreationInfo info;
        info.type = ImageType::Static;
        info.format = sourceData->format();
        info.view = sourceData->type();
        info.m_numSlices = sourceData->numSlices();
        info.m_numMips = sourceData->numMips();
        info.width = sourceData->width();
        info.height = sourceData->height();
        info.m_depth = sourceData->depth();
        info.label = m_debugLabel;

        // get the persistent data
        void* persistentData = (void*)sourceData->persistentData();

        // setup source data
        base::Array<SourceData> sourceMips;
        sourceMips.reserve(sourceData->mips().size());
        uint32_t totalMemorySize = 0;
        for (auto& blobMip : sourceData->mips())
        {
            auto& sourceMip = sourceMips.emplaceBack();

            // if the data was compressed with Crunch unpack it
            if (blobMip.m_compressed)
            {
                // decompress
                auto compressedData  = base::OffsetPtr(persistentData, blobMip.dataOffset);
                uint32_t ddsDataSize = blobMip.dataSize;
                auto ddsData  = crn_decompress_crn_to_dds(compressedData, ddsDataSize);
                if (!ddsData)
                {
                    TRACE_ERROR("Failed to decompress Crunched texture [{}x{}x{}], data size {}",
                                blobMip.width, blobMip.height, blobMip.dataSize, blobMip.dataSize);
                    return false;
                }

                /*// read the DDS
                crn_texture_desc ddsDesc;
                crn_uint32* imageData[1] = {nullptr};
                if (!crn_decompress_dds_to_images(ddsData, ddsDataSize, imageData, ddsDesc))
                {
                    TRACE_ERROR("Failed to load decompressed Crunched texture [{}x{}x{}], data size {}, DDS size {}",
                                blobMip.width, blobMip.height, blobMip.dataSize, blobMip.dataSize, ddsDataSize);
                    return nullptr;
                }*/

                // free temp data
                //crn_free_block(ddsData);

                // create fake buffer
                auto freeFunc = [ddsData](void* ptr, uint32_t size) { crn_free_block(ddsData); };
                auto dataBuffer = base::RefNew<base::mem::Buffer>((char*)ddsData + 128, ddsDataSize - 128, freeFunc);
                sourceMip.offset = 0;
                sourceMip.data = dataBuffer;
                sourceMip.size = ddsDataSize - 128;
                totalMemorySize += blobMip.dataSize;

            }
            else
            {
                // use the direct data
                sourceMip.data = (base::OffsetPtr(persistentData, blobMip.dataOffset), blobMip.dataSize);
                sourceMip.offset = 0;
                sourceMip.size = blobMip.dataSize;
                totalMemorySize += blobMip.dataSize;
            }
        }

        // create the image
        auto imageView = driver->createImage(info, sourceMips.typedData());
        if (imageView.empty())
            return false;

        // create the sampler
        auto sampler = SelectSamplerForTextureGroup(driver, sourceData->textureGroupName(), sourceData->numMips() > 1);
        m_mainView = imageView.createSampledView(sampler);
        return true;
    }

    // data not created
    return false;
}

//---

Sampler Texture::SelectSamplerForTextureGroup(IDevice* drv, base::StringID textureGroupName, bool hasMipmaps)
{
    // get the texture group data
    auto& textureGroup = GetTextureGroup(textureGroupName);
            
    // setup wrapping
    SamplerCreationInfo setupInfo;
    setupInfo.addresModeU = textureGroup.m_wrapU ? AddressMode::Wrap : AddressMode::Clamp;
    setupInfo.addresModeV = textureGroup.m_wrapV ? AddressMode::Wrap : AddressMode::Clamp;
    setupInfo.addresModeW = textureGroup.m_wrapW ? AddressMode::Wrap : AddressMode::Clamp;

    // setup filtering
    if (textureGroup.m_filteringMode == TextureFilteringMode::Point)
    {
        setupInfo.mipmapMode = hasMipmaps ? MipmapFilterMode::Nearest : MipmapFilterMode::None;
        setupInfo.minFilter = FilterMode::Nearest;
        setupInfo.magFilter = FilterMode::Nearest;
    }
    else if (!hasMipmaps || textureGroup.m_filteringMode == TextureFilteringMode::Bilinear)
    {
        setupInfo.mipmapMode = hasMipmaps ? MipmapFilterMode::Nearest : MipmapFilterMode::None;
        setupInfo.minFilter = FilterMode::Linear;
        setupInfo.magFilter = FilterMode::Linear;
    }
    else if (hasMipmaps && textureGroup.m_filteringMode == TextureFilteringMode::Trilinear)
    {
        setupInfo.mipmapMode = MipmapFilterMode::Linear;
        setupInfo.minFilter = FilterMode::Linear;
        setupInfo.magFilter = FilterMode::Linear;
    }
    else if (hasMipmaps && textureGroup.m_filteringMode == TextureFilteringMode::Aniso)
    {
        setupInfo.maxAnisotropy = std::min<uint8_t>(textureGroup.maxAnisotropy, 16); // TODO
        setupInfo.mipmapMode = MipmapFilterMode::Linear;
        setupInfo.minFilter = FilterMode::Linear;
        setupInfo.magFilter = FilterMode::Linear;
    }

    // create sampler
    return drv->createSampler(setupInfo);
}

//---
#endif

END_BOOMER_NAMESPACE(rendering)