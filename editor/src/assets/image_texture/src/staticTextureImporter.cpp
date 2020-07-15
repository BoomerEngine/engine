/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#include "build.h"
#include "imageCompression.h"
#include "staticTextureImporter.h"

#include "base/io/include/ioFileHandle.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"
#include "base/app/include/localServiceContainer.h"
#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCookingInterface.h"
#include "base/containers/include/inplaceArray.h"
#include "base/image/include/imageView.h"
#include "assets/image_loader/include/freeImageLoader.h"
#include "base/resource/include/resourceTags.h"

namespace rendering
{

    //---

    RTTI_BEGIN_TYPE_CLASS(StaticTextureCompressionConfiguration);
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
        // TODO: streaming settings
        // TODO: per-platform settings (ie. console texture bias)
    RTTI_END_TYPE();

    StaticTextureCompressionConfiguration::StaticTextureCompressionConfiguration()
    {}

    ImageCompressionSettings StaticTextureCompressionConfiguration::loadSettings() const
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

    RTTI_BEGIN_TYPE_CLASS(StaticTextureFromImageImporter);
        RTTI_METADATA(base::res::ResourceCookedClassMetadata).addClass<StaticTexture>();
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtensions("bmp;dds;png;jpg;jpeg;jp2;jpx;tga;tif;tiff;hdr;exr;ppm;pbm;psd;xbm;nef;xpm;gif;webp");
        RTTI_METADATA(base::res::ResourceImporterConfigurationClassMetadata).configurationClass<StaticTextureCompressionConfiguration>();
    RTTI_END_TYPE();

    StaticTextureFromImageImporter::StaticTextureFromImageImporter()
    {}

    static base::StringView<char> GetTextureSuffix(base::StringView<char> path)
    {
        auto fileName = path.afterLastOrFull("/").beforeFirstOrFull(".");
        if (fileName.empty())
            return base::StringBuf::EMPTY();

        return fileName.afterLast("_");
    }

    base::res::ResourcePtr StaticTextureFromImageImporter::importResource(base::res::IResourceImporterInterface& importer) const
    {
        // load the source data into memory
        // NOTE: we don't use source asset cache since image files are usually not imported many times over
        const auto& importPath = importer.queryImportPath();
        auto bufferData = importer.loadSourceFileContent(importPath);
        if (!bufferData)
        {
            TRACE_ERROR("Unable to load file content from '{}'", importer.queryImportPath());
            return nullptr;
        }

        // load the image itself
        auto imageData = base::image::LoadImageWithFreeImage(bufferData.data(), bufferData.size());
        if (!imageData)
        {
            TRACE_ERROR("Unable to load image from '{}'", importer.queryImportPath());
            return nullptr;
        }

        // get compression configuration
        auto config = importer.queryConfigration<StaticTextureCompressionConfiguration>();

        // get settings 
        auto settings = config->loadSettings();
        settings.m_suffix = base::StringBuf(GetTextureSuffix(importPath));

        // TODO: reuse compressed data if for some reason the format is the same

        // compile the image
        auto compressedData = CompressImage(imageData->view(), settings, importer);
        if (!compressedData)
        {
            TRACE_ERROR("Image compression failed for from '{}'", importPath);
            return nullptr;
        }

        // TODO: split data into streamable/persistent part

        // create the static texture
        base::res::AsyncBuffer streamingData;
        return base::CreateSharedPtr<StaticTexture>(std::move(compressedData->data), std::move(streamingData), std::move(compressedData->mips), compressedData->info);
    }

    //---
    
} // rendering
