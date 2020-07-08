/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "imageCompression.h"
#include "staticTextureCooker.h"

#include "base/io/include/ioFileHandle.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/streamBinaryWriter.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/containers/include/inplaceArray.h"
#include "base/image/include/imageView.h"

namespace rendering
{

    //---

    RTTI_BEGIN_TYPE_CLASS(StaticTextureFromImageManifest);
        RTTI_METADATA(base::res::ResourceManifestExtensionMetadata).extension("st.meta");
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

    StaticTextureFromImageManifest::StaticTextureFromImageManifest()
    {}

    ImageCompressionSettings StaticTextureFromImageManifest::loadSettings() const
    {
        ImageCompressionSettings ret;
        ret.m_compressionMasking = m_compressionMasking;
        ret.m_compressionMode = m_compressionMode;
        ret.m_compressionQuality = m_compressionQuality;
        ret.m_contentAlphaMode = m_contentAlphaMode;
        ret.m_contentColorSpace = m_contentColorSpace;
        ret.m_mipmapMode = m_mipmapMode;
        ret.m_contentType = m_contentType;
        return ret;
    }

    //---

    RTTI_BEGIN_TYPE_CLASS(StaticTextureFromImageCooker);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<StaticTexture>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceClass<base::image::Image>();
    RTTI_END_TYPE();

    StaticTextureFromImageCooker::StaticTextureFromImageCooker()
    {
    }

    void StaticTextureFromImageCooker::reportManifestClasses(base::Array<base::SpecificClassType<base::res::IResourceManifest>>& outManifestClasses) const
    {
        outManifestClasses.pushBackUnique(StaticTextureFromImageManifest::GetStaticClass());
    }
    
    static base::StringView<char> GetTextureSuffix(base::StringView<char> path)
    {
        auto fileName = path.afterLastOrFull("/").beforeFirstOrFull(".");
        if (fileName.empty())
            return base::StringBuf::EMPTY();

        return fileName.afterLast("_");
    }

    base::res::ResourceHandle StaticTextureFromImageCooker::cook(base::res::IResourceCookerInterface& cooker) const
    {
        auto importPath = cooker.queryResourcePath();

        // load the source image
        auto sourceImage = cooker.loadDependencyResource<base::image::Image>(importPath);
        if (!sourceImage)
        {
            TRACE_ERROR("Unable to load source image from '{}'", importPath);
            return nullptr;
        }

        // load compression manifest
        auto manifest = cooker.loadManifestFile<StaticTextureFromImageManifest>();

        // get settings
        auto settings = manifest->loadSettings();
        settings.m_suffix = base::StringBuf(GetTextureSuffix(importPath.view()));

        // compile the image
        auto compressedData = CompressImage(sourceImage->view(), settings, cooker);
        if (!compressedData)
        {
            TRACE_ERROR("Image compression failed for from '{}'", importPath);
            return nullptr;
        }

        // create the static texture
        base::res::AsyncBuffer streamingData;
        return base::CreateSharedPtr<StaticTexture>(std::move(compressedData->data), std::move(streamingData), std::move(compressedData->mips), compressedData->info);
    }
        

        /*
        static base::StringID GetBestTextureGroupForFormat(base::StringView<char> sourceFile, const base::image::Image& img)
        {
            // get basic info about the image
            auto hasAlpha = (img.channels() == 4);
            auto isScalar = (img.channels() == 1);
            auto isHDR = (img.format() == base::image::PixelFormat::Float16_Raw || img.format() == base::image::PixelFormat::Float32_Raw);

            // hdr content
            if (isHDR)
            {
                if (isScalar)
                    return "HDRScalar"_id;
                else
                    return "HDRColor"_id;
            }

            // list compatible texture groups for given image suffix
            base::InplaceArray<base::StringID, 4> comaptibleTextureGroupNames;
            //TextureGr

            // single channel scalar image
            if (isScalar)
                return "DefaultScalar"_id;
            else
                return "Default"_id;
        }

        static void GenerateMipmaps(TextureMipmapGenerationMode mipmapMode, TextureContentType contentType, base::Array<base::image::ImagePtr>& mipChain)
        {
            ASSERT(mipmapMode != TextureMipmapGenerationMode::Auto);

            // clear existing mipmaps
            mipChain.resize(1);

            // no mipmaps to generate
            if (mipmapMode == TextureMipmapGenerationMode::NoMipmaps)
                return;

            // determine the color space for the image
            auto colorSpace = GetColorSpaceForContentType(contentType);

            // downsample image until we reach 1x1x1 image
            auto curImg = mipChain.back();
            auto width = curImg->width();
            auto height = curImg->height();
            auto depth = curImg->depth();
            uint32_t mipIndex = 0;
            for (;;)
            {
                // process the downsampling
                switch (mipmapMode)
                {
                case TextureMipmapGenerationMode::BoxFilter:
                    //curImg = base::image::BoxFilter(curImg, colorSpace);
                    break;

                case TextureMipmapGenerationMode::Mitchel:
                    // TODO!
                    //curImg = base::image::BoxFilter(curImg, colorSpace);
                    break;
                }

                // no more images to filter
                if (!curImg)
                    break;

                // add downsampled image to the mip chain
                mipChain.pushBack(curImg);

                // reduce size of the image for the next mip
                // NOTE: we clamp to 1 since the image does not have to be a square
                width = std::max<uint16_t>(1, width / 2);
                height = std::max<uint16_t>(1, height / 2);
                depth = std::max<uint16_t>(1, depth / 2);
            }
        }*/

        /*static rendering::ImageFormat DetermineBestFormat(ImageCompressionFormat contentType, uint32_t width, uint32_t height, uint32_t numChannels, bool hasAlpha, bool hasFloatingPointData, bool allowCompression)
        {
            // can we compress this image with DXT blocks ?
            auto canUseDXT = true;//((width & 3) == 0) && ((height & 3) == 0);

            // if floating point data is used preserve the format
            if (hasFloatingPointData)
            {
                if (numChannels == 1)
                    return allowCompression ? rendering::ImageFormat::R16F : rendering::ImageFormat::R32F;
                else if (numChannels == 2)
                    return allowCompression ? rendering::ImageFormat::RG16F : rendering::ImageFormat::RG32F;
                else
                    return allowCompression ? rendering::ImageFormat::RGBA16F : rendering::ImageFormat::RGBA32F;
            }

            // choose format based on the content and image data
            if (contentType == TextureContentType::ColorLinear || contentType == TextureContentType::Other)
            {
                if (!hasAlpha || numChannels == 3)
                    return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC1_UNORM : rendering::ImageFormat::RGBA8_UNORM;
                else if (numChannels == 4)
                    return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC3_UNORM : rendering::ImageFormat::RGBA8_UNORM;
            }
            else if (contentType == TextureContentType::ColorSRGB)
            {
                if (!hasAlpha || numChannels == 3)
                    return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC1_SRGB : rendering::ImageFormat::SRGB8;
                else if (numChannels == 4)
                    return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC3_SRGB : rendering::ImageFormat::SRGB8;
            }
            else if (contentType == TextureContentType::ColorHDR)
            {
                return rendering::ImageFormat::RGBA16F;
            }
            else if (contentType == TextureContentType::TangentNormals)
            {
                if (hasAlpha)
                    return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC3_UNORM : rendering::ImageFormat::RGBA8_UNORM;
                else
                    return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC5_UNORM : rendering::ImageFormat::RG8_UNORM;
            }
            else if (contentType == TextureContentType::WorldNormals)
            {
                if (hasAlpha)
                    return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC3_UNORM : rendering::ImageFormat::RGBA8_UNORM;
                else
                    return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC3_UNORM : rendering::ImageFormat::RGBA8_UNORM;
            }
            else if (contentType == TextureContentType::AlphaMask)
            {
                return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC3_UNORM : rendering::ImageFormat::R8_UNORM;
            }
            else if (contentType == TextureContentType::ScalarLinear)
            {
                return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC1_UNORM : rendering::ImageFormat::R8_UNORM;
            }
            else if (contentType == TextureContentType::ScalarSRGB)
            {
                return (allowCompression && canUseDXT) ? rendering::ImageFormat::BC1_SRGB : rendering::ImageFormat::SRGB8;
            }
            else if (contentType == TextureContentType::ScalarHDR)
            {
                return rendering::ImageFormat::R16F;
            }

            TRACE_ERROR("Unable to translate texture format");
            return rendering::ImageFormat::RGBA8_UNORM;
        }*/

#if 0
        /// compute amount of memory needed for the image
        static void ComputeMemorySettings(ImageFormat format, const base::image::ImageView& size, uint32_t& outMemoryNeeded, uint32_t& outRowPitch, uint32_t& outSlicePitch, uint8_t& outBlockSize)
        {
            auto bpp = GetImageFormatInfo(format).m_bitsPerPixel;
            if (GetImageFormatInfo(format).m_compressed)
            {
                auto numBlocksX = std::max<uint32_t>(1, size.width() / 4);
                auto numBlocksY = std::max<uint32_t>(1, size.height() / 4);

                ASSERT(size.depth() == 1);
                outRowPitch = numBlocksX * (2 * bpp);
                outSlicePitch = outRowPitch * numBlocksY;
                outMemoryNeeded = outSlicePitch;
                outBlockSize = 4;
            }
            else
            {
                ASSERT(bpp >= 8);
                outRowPitch = (size.width() * bpp) / 8;
                outSlicePitch = outRowPitch * size.height();
                outMemoryNeeded = outSlicePitch * size.depth();
                outBlockSize = 1;
            }
        }

    

        /// make sure the image has right format for the packing, usually this changes the pixel format and channel count
        /// we don't care here about any data getting lost, if a format was chosen by somebody than it was chosen
        static base::image::ImagePtr PrepareImageForConversion(ImageFormat format, const base::image::ImagePtr& sourceImage)
        {
            base::image::ImagePtr imagePtr = sourceImage;;

            auto requiredPixelFormat = GetImagePixelFormat(format);
            auto requiredChannelCount = GetImageFormatInfo(format).m_numComponents;

            // convert to required channel count
            //if (requiredChannelCount != sourceImage->channels())
                //imagePtr = base::image::ConvertChannels(imagePtr, requiredChannelCount, nullptr, base::Vector4(0, 0, 0, 1));

            // convert to required format for packing
            //if (requiredPixelFormat != sourceImage->pixelFormat())
                //imagePtr = base::image::ConvertPixelFormat(imagePtr, requiredPixelFormat);

            return imagePtr;
        }

        // pack data that has the same row pitch
        static bool PackDataIdeal(const base::image::ImagePtr& sourceImage, uint8_t* outData, const TextureDataMipMap& mipInfo)
        {
            ASSERT(sourceImage->layout().m_rowPitch == mipInfo.m_rowPitch);

            // copy slices
            auto writePtr  = outData;
            auto readPtr  = (const uint8_t*)sourceImage->pixelBuffer();
            auto readPitch = sourceImage->layout().m_layerPitch;
            for (uint32_t i = 0; i < mipInfo.m_depth; ++i)
            {
                auto copySize = std::min<uint32_t>(readPitch, mipInfo.m_slicePitch);
                memcpy(writePtr, readPtr, copySize);
                writePtr += mipInfo.m_slicePitch;
                readPtr += readPitch;
            }

            // no errors here
            return true;
        }

        // pack data from image into the output buffer, used when there's no real difference in format (maybe in pitch etc)
        static bool PackDataSimple(ImageFormat format, const base::image::ImagePtr& sourceImage, base::Buffer& outData, TextureDataMipMap& outMipInfo)
        {
            ASSERT(sourceImage->size().width == outMipInfo.width);
            ASSERT(sourceImage->size().height == outMipInfo.height);
            ASSERT(sourceImage->size().m_depth == outMipInfo.m_depth);

            // compute image packing
            ComputeMemorySettings(format, sourceImage->size(), outMipInfo.dataSize, outMipInfo.m_rowPitch, outMipInfo.m_slicePitch, outMipInfo.block);

            // allocate output data
            outData.reset(outMipInfo.dataSize, nullptr);
            if (outData.empty())
            {
                TRACE_ERROR("Unable to allocate memory buffer required to compress the texture, required size: {}", MemSize(outMipInfo.dataSize));
                return false;
            }

            // for some formats the source and destination memory layout can be ideal, in that case we can memcpy
            if (sourceImage->layout().m_rowPitch == outMipInfo.m_rowPitch)
                return PackDataIdeal(sourceImage, outData.data(), outMipInfo);

            auto readRowPitch = sourceImage->layout().m_rowPitch;
            auto readSlicePitch = sourceImage->layout().m_layerPitch;

            // copy data row by row
            auto writeSlicePtr  = outData.data();
            auto readSlicePtr  = (const uint8_t*)sourceImage->pixelBuffer();
            for (uint32_t i = 0; i < outMipInfo.m_depth; ++i)
            {
                auto writePtr  = writeSlicePtr;
                auto readPtr  = readSlicePtr;

                for (uint32_t j = 0; j < outMipInfo.height; ++j)
                {
                    auto copySize = std::min<uint32_t>(outMipInfo.m_rowPitch, readRowPitch);
                    memcpy(writePtr, readPtr, copySize);
                    writePtr += outMipInfo.m_rowPitch;
                    readPtr += readRowPitch;
                }

                writeSlicePtr += outMipInfo.m_slicePitch;
                readSlicePtr += readSlicePitch;
            }

            // packed
            return true;
        }

        // pack data using Crunch texture packer
        static bool PackDataWithCrunch(ImageFormat format, char qualityBias, const base::image::ImagePtr& sourceImage, base::Buffer& outData, TextureDataMipMap& outMipInfo)
        {
            ASSERT(sourceImage->size().width == outMipInfo.width);
            ASSERT(sourceImage->size().height == outMipInfo.height);
            ASSERT(sourceImage->size().m_depth == outMipInfo.m_depth);

            // compute image packing
            ComputeMemorySettings(format, sourceImage->size(), outMipInfo.dataSize, outMipInfo.m_rowPitch, outMipInfo.m_slicePitch, outMipInfo.block);

            // setup Crunch compression setting
            crn_comp_params comp_params;
            comp_params.width = sourceImage->size().width;
            comp_params.height = sourceImage->size().height;
            comp_params.m_levels = sourceImage->size().m_depth;
            //comp_params.set_flag(cCRNCompFlagHierarchical, true); // for now
            comp_params.set_flag(cCRNCompFlagHierarchical, false);
            comp_params.set_flag(cCRNCompFlagQuick, true);
            comp_params.m_file_type = cCRNFileTypeCRN;
            comp_params.m_quality_level = (uint8_t)std::clamp<int>(60 + (int)qualityBias, 0, 255);
            comp_params.m_num_helper_threads = 3;

            // format dependent stuff
            switch (format)
            {
            case ImageFormat::BC1_UNORM:
            case ImageFormat::BC1_SRGB:
            {
                auto isFormatSRGB = (format == ImageFormat::BC1_SRGB);
                auto hasAlpha = (sourceImage->size().m_channels == 4);
                comp_params.set_flag(cCRNCompFlagDXT1AForTransparency, hasAlpha);
                comp_params.set_flag(cCRNCompFlagPerceptual, isFormatSRGB);
                comp_params.format = cCRNFmtDXT1;
                break;
            }

            case ImageFormat::BC2_UNORM:
            case ImageFormat::BC2_SRGB:
            {
                auto isFormatSRGB = (format == ImageFormat::BC1_SRGB);
                comp_params.set_flag(cCRNCompFlagPerceptual, isFormatSRGB);
                comp_params.format = cCRNFmtDXT3;
                break;
            }

            case ImageFormat::BC3_UNORM:
            case ImageFormat::BC3_SRGB:
            {
                auto isFormatSRGB = (format == ImageFormat::BC1_SRGB);
                comp_params.set_flag(cCRNCompFlagPerceptual, isFormatSRGB);
                comp_params.format = cCRNFmtDXT5;
                break;
            }

            case ImageFormat::BC4_UNORM:
            {
                auto isFormatSRGB = false;// (format == ImageFormat::BC4_PACKED_SRGB);
                comp_params.set_flag(cCRNCompFlagPerceptual, isFormatSRGB);
                comp_params.format = cCRNFmtDXT5A;
                break;
            }

            case ImageFormat::BC5_UNORM:
            {
                auto isFormatSRGB = false;// (format == ImageFormat::BC5_PACKED_SRGB);
                comp_params.set_flag(cCRNCompFlagPerceptual, isFormatSRGB);
                comp_params.format = cCRNFmtDXN_XY;
                break;
            }

            default:
            {
                TRACE_ERROR("Invalid texture format for compressing with CRUNCH");
                return false;
            }
            }

            // get data to 4-channel format
            auto crunchCompatibleImage = base::image::ConvertPixelFormat(
                base::image::ConvertChannels(sourceImage, 4), base::image::PixelFormat::Uint8_Norm);

            // make it fast under debug
    #ifdef BUILD_DEBUG
            comp_params.m_quality_level = 255;
    #endif

            // set image data
            comp_params.m_pImages[0][0] = (const uint32_t*)crunchCompatibleImage->pixelBuffer();

            // compress the image !
            uint32_t outputDataSize = 0;
            uint32_t actualQualityLevel = 0;
            float actualBitRate = 0;
            void* compressedDataPtr = crn_compress(comp_params, outputDataSize, &actualQualityLevel, &actualBitRate);
            if (!compressedDataPtr)
            {
                TRACE_ERROR("Failed to compress texture using CRUNCH");
                return false;
            }

            // stats
            TRACE_INFO("Compress texture data [{}x{}x{}], final data size ({}) {} bits/pixel (at quiality level {})",
                outMipInfo.width, outMipInfo.height, outMipInfo.m_depth, MemSize(outputDataSize), MemSize(actualBitRate), actualQualityLevel);

            // setup stuff
            outMipInfo.dataSize = outputDataSize;
            outMipInfo.m_compressed = true;

            // create the output buffer
            outData.reset(outputDataSize, compressedDataPtr);
            if (outData.empty())
            {
                TRACE_ERROR("Unable to allocate memory buffer required to compress the texture, required size: {}", MemSize(outMipInfo.dataSize));
                return false;
            }

            // free the compressed data
            crn_free_block(compressedDataPtr);
            return true;
        }

        TextureDataPtr CompileTextureBlob(const TextureBuildData& sourceData)
        {
            base::Task task(2, "Creating data blob");

            auto startTime = base::NativeTimePoint::Now();

            // no source images
            if (sourceData.m_slices.empty())
            {
                TRACE_ERROR("No texture data to compile into blob");
                return nullptr;
            }

            // extract data count
            auto numSlices = range_cast<uint16_t>(sourceData.m_slices.size());
            auto numMips = range_cast<uint8_t>(sourceData.m_slices[0].m_mips.size());
            if (!numMips)
            {
                TRACE_ERROR("No texture data to compile into blob");
                return nullptr;
            }

            // determine if image has alpha
            auto hasAlpha = (sourceData.m_numChannels == 4);

            // if we are using floating point format we may need HDR stuff
            bool hasFloatingPointContent = (sourceData.m_pixelFormat == base::image::PixelFormat::Float16_Raw || sourceData.m_pixelFormat == base::image::PixelFormat::Float32_Raw);

            // determine compiled format for the images
            auto rootImageSize = sourceData.m_slices[0].m_mips[0]->size();
            auto compiledFormat = DetermineBestFormat(sourceData.m_contentType, rootImageSize.width, rootImageSize.height, sourceData.m_numChannels, hasAlpha, hasFloatingPointContent, sourceData.m_allowCompression);
            ASSERT(compiledFormat != ImageFormat::UNKNOWN);

            struct TempMipmap
            {
                TextureDataMipMap m_mipInfo;
                base::Buffer data;
            };

            // setup output data
            TRACE_INFO("Compiling texture blob {} (group {}), [{}x{}x{}], {}, {} mips, {} slices",
                base::reflection::GetEnumValueName(sourceData.type),
                sourceData.m_textureGroupName.c_str(),
                sourceData.width, sourceData.height, sourceData.m_depth,
                base::reflection::GetEnumValueName(compiledFormat),
                numMips, numSlices);

            // pack each slice
            base::Array<TempMipmap*> tempMipmaps;
            tempMipmaps.reserve(numMips * numSlices);
            {
                base::Task task(numMips * numSlices, "Compressing mipmaps");
                for (auto& srcSlice : sourceData.m_slices)
                {
                    for (auto& srcMip : srcSlice.m_mips)
                    {
                        auto mip  = MemNew(TempMipmap);
                        ProcessSingleMip(compiledFormat, sourceData.m_compressionQualityBias, srcMip, mip->data, mip->m_mipInfo);
                        tempMipmaps.pushBack(mip);
                        task.advance();
                    }
                }
            }

            // TODO: tiling & swizzling for consoles
            uint32_t dataAlignment = 0x100;

            // compute size of the data needed to pack all the data
            // this also allocates the mips within the data buffer by setting the dataOffset field
            uint32_t totalDataSize = 0;
            for (auto mip  : tempMipmaps)
            {
                mip->m_mipInfo.dataOffset = base::Align(totalDataSize, dataAlignment);
                totalDataSize = mip->m_mipInfo.dataOffset + mip->m_mipInfo.dataSize;
            }

            // align the size to the next required slot
            // NOTE: on consoles this may be a lot (like a page size)
            totalDataSize = base::Align(totalDataSize, dataAlignment);

            // create output data
            base::Buffer compiledData;
            base::Array<TextureDataMipMap> compiledMips;

            // copy the data
            compiledData.reset(totalDataSize, nullptr);
            compiledMips.reserve(tempMipmaps.size());
            for (auto mip  : tempMipmaps)
            {
                // copy data to the output data buffer
                auto targetPtr  = base::OffsetPtr(compiledData.data(), mip->m_mipInfo.dataOffset);
                memcpy(targetPtr, mip->data.data(), mip->m_mipInfo.dataSize);

                // copy mip settings
                compiledMips.pushBack(mip->m_mipInfo);
            }

            // cleanup
            tempMipmaps.clearPtr();

            // create final data holder
            auto ret = base::CreateSharedPtr<data::TextureData>(sourceData.m_textureGroupName,
                std::move(compiledData), std::move(compiledMips),
                compiledFormat, sourceData.type,
                range_cast<uint8_t>(numMips),
                range_cast<uint16_t>(numSlices),
                range_cast<uint16_t>(sourceData.width),
                range_cast<uint16_t>(sourceData.height),
                range_cast<uint16_t>(sourceData.m_depth));

            // stats
            TRACE_INFO("Texture blob for group '{}' created, {}", sourceData.m_textureGroupName, TimeInterval(startTime.timeTillNow().toSeconds()));
            return ret;
        }

        /// compress/pack single image
        static bool ProcessSingleMip(ImageFormat format, char qualityBias, const base::image::ImagePtr& sourceImageUnchecked, base::Buffer& outData, TextureDataMipMap& outMipInfo)
        {
            // convert image to proper format usable for conversion
            auto sourceImage = PrepareImageForConversion(format, sourceImageUnchecked);

            // setup output information
            outMipInfo.width = sourceImage->size().width;
            outMipInfo.height = sourceImage->size().height;
            outMipInfo.m_depth = sourceImage->size().m_depth;
            outMipInfo.block = 0;
            outMipInfo.m_rowPitch = 0;
            outMipInfo.m_slicePitch = 0;
            outMipInfo.dataSize = 0;
            outMipInfo.dataOffset = 0;

            // do the packing
            if (GetImageFormatInfo(format).m_compressed)
                return PackDataWithCrunch(format, qualityBias, sourceImage, outData, outMipInfo);
            else
                return PackDataSimple(format, sourceImage, outData, outMipInfo);
        }

        void GenerateMipmaps(TextureMipmapGenerationMode mipmapMode, TextureContentType contentType, base::Array<base::image::ImagePtr>& mipChain)
        {
            ASSERT(mipmapMode != TextureMipmapGenerationMode::Auto);

            // clear existing mipmaps
            mipChain.resize(1);

            // no mipmaps to generate
            if (mipmapMode == TextureMipmapGenerationMode::NoMipmaps)
                return;

            // determine the color space for the image
            auto colorSpace = GetColorSpaceForContentType(contentType);

            // downsample image until we reach 1x1x1 image
            auto curImg = mipChain.back();
            auto width = curImg->size().width;
            auto height = curImg->size().height;
            auto depth = curImg->size().m_depth;
            uint32_t mipIndex = 0;
            for (;;)
            {
                // process the downsampling
                switch (mipmapMode)
                {
                case TextureMipmapGenerationMode::BoxFilter:
                    curImg = base::image::BoxFilter(curImg, colorSpace);
                    break;

                case TextureMipmapGenerationMode::Mitchel:
                    // TODO!
                    curImg = base::image::BoxFilter(curImg, colorSpace);
                    break;
                }

                // no more images to filter
                if (!curImg)
                    break;

                // add downsampled image to the mip chain
                mipChain.pushBack(curImg);

                // reduce size of the image for the next mip
                // NOTE: we clamp to 1 since the image does not have to be a square
                width = std::max<uint16_t>(1, width / 2);
                height = std::max<uint16_t>(1, height / 2);
                depth = std::max<uint16_t>(1, depth / 2);
            }
        }

        /*rendering::TextureDataPtr BakeDataBlob(base::StringID textureGroupName, const rendering::ImageViewType type, uint32_t numSourceMip, uint32_t numSourceSlices, const base::Array<base::image::ImagePtr>& sourceImages, bool allowMipmaps, char compressionQualityBias)
        {        
            // validate source
            auto numExpectedMips = numSourceMip * numSourceSlices;
            ASSERT_EX(numExpectedMips == sourceImages.size(), "Not enough source images passed as source");
            ASSERT_EX(numExpectedMips > 0, "Cannot initialize empty texture");

            // get texture group data
            auto& textureGroup = rendering::runtime::GetTextureGroup(textureGroupName);

            // get the root image
            auto& rootImage = *sourceImages[0];

            // generate mipmaps, only if not present already
            auto mipmapGenerator = textureGroup.mipmapMode;
            if (mipmapGenerator == TextureMipmapGenerationMode::Auto)
                mipmapGenerator = DetermineBestMipMode(textureGroup.m_contentType);
            if (!allowMipmaps)
                mipmapGenerator = TextureMipmapGenerationMode::NoMipmaps;

            // translate texture type
            rendering::data::TextureBuildData buildData;
            buildData.m_textureGroupName = textureGroupName;
            buildData.type = type;
            buildData.m_contentType = textureGroup.m_contentType;
            buildData.width = rootImage.size().width;
            buildData.height = rootImage.size().height;
            buildData.m_depth = rootImage.size().m_depth;
            buildData.m_allowCompression = textureGroup.m_allowCompression && (buildData.width > 64 && buildData.height > 64);
            buildData.m_allowStreaming = textureGroup.m_allowStreaming;
            buildData.m_compressionQualityBias = compressionQualityBias + textureGroup.m_compressionQualityBias;
            buildData.m_numChannels = rootImage.size().m_channels;

            // setup source data
            uint32_t mipIndex = 0;
            buildData.m_slices.reserve(numSourceSlices);
            {
                base::Task task(numSourceSlices, "Generate mipmaps");
                for (uint32_t i = 0; i < numSourceSlices; ++i)
                {
                    auto &buildSlice = buildData.m_slices.emplaceBack();

                    // copy existing mipmaps
                    buildSlice.m_mips.reserve(numSourceMip);
                    for (uint32_t j = 0; j < numSourceMip; ++j)
                    {
                        auto sourceImage = sourceImages[mipIndex++];
                        buildSlice.m_mips.pushBack(sourceImage);
                    }

                    // run mipmap generator
                    rendering::data::GenerateMipmaps(mipmapGenerator, buildData.m_contentType, buildSlice.m_mips);
                    task.advance();
                }
            }

            // build the texture
            return rendering::data::CompileTextureBlob(buildData);
        }*/

        /*static base::image::ImagePtr DropMips(const TextureGroup& textureGroup, const base::image::ColorSpace colorSpace, uint8_t mipBias, const base::image::ImagePtr& imagePtr)
        {
            auto ret = imagePtr;

            // count how many mips we have to drop
            uint32_t numDropMips = 0;
            auto size = imagePtr->size();
            auto maxSize = textureGroup.m_maxSize;
            while (size.width > maxSize || size.height > maxSize || size.m_depth > maxSize)
            {
                size.width = std::max<uint16_t>(size.width >> 1, 1);
                size.height = std::max<uint16_t>(size.height >> 1, 1);
                size.m_depth = std::max<uint16_t>(size.m_depth >> 1, 1);
                numDropMips += 1;
            }

            // texture may override
            numDropMips = std::max<uint32_t>(numDropMips, mipBias);
            if (numDropMips > 0)
            {
                base::Task task(numDropMips, "Downsample");

                TRACE_INFO("Applying mip bias {}", numDropMips);
                for (uint32_t i = 0; i < numDropMips; ++i)
                {
                    if (!imagePtr)
                    {
                        TRACE_ERROR("Failed to downsample image before cooking");
                        return nullptr;
                    }

                    if (imagePtr->size().width == 1 && imagePtr->size().height == 1 && imagePtr->size().m_depth)
                        break;

                    ret = base::image::BoxFilter(ret, colorSpace);
                    task.advance();
                }

                TRACE_INFO("Resulting texture size after mip-bias: {}x{}x{}", imagePtr->size().width, imagePtr->size().height, imagePtr->size().m_depth);
            }

            return ret;
        }*/

        //--
#endif
        /*base::res::ResourceHandle ImageTextureCooker::cook(base::res::IResourceCookerInterface& cooker) const
        {
            // load texture manifest
            auto textureManifest = cooker.loadManifestFile<ImageCompressionManifest>();

            // load content to buffer
            auto imagePath = cooker.queryResourcePath();
            auto imagePtr = cooker.loadDependencyResource<base::image::Image>(imagePath);
            if (!imagePtr)
            {
                TRACE_ERROR("Unable to load source image from '{}' so no texture cooking possible", imagePath);
                return false;
            }

            // we may have resigned by now...
            if (cooker.isCanceled())
                return false;

            // load manifest
            auto manifest = cooker.loadManifestFile<ImageCompressionManifest>();

            // compress the image
            const auto data = CompressImage(imagePtr->view(), *manifest);
            if (!data)
            {
                TRACE_ERROR("Unable to compress source image from '{}' into a texture", imagePath);
                return false;
            }

            // extract data for streamed mips
            // TODO
            base::res::AsyncBuffer streamedData;

            // create final texture
            return base::CreateSharedPtr<ImageTexture>(std::move(data->data), std::move(streamedData), std::move(data->mips), data->info);
        }*/

        //---
    
} // rendering
