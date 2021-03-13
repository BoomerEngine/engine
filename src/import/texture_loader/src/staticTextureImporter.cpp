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

#include "core/io/include/fileHandle.h"
#include "core/image/include/image.h"
#include "core/image/include/imageUtils.h"
#include "core/app/include/localServiceContainer.h"
#include "core/resource/include/resource.h"
#include "core/containers/include/inplaceArray.h"
#include "core/image/include/imageView.h"
#include "core/image/include/freeImageLoader.h"
#include "core/resource/include/tags.h"
#include "engine/texture/include/staticTexture2D.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//---

RTTI_BEGIN_TYPE_CLASS(StaticTextureCompressionConfiguration);
    RTTI_OLD_NAME("rendering::StaticTextureCompressionConfiguration");
    RTTI_CATEGORY("Content");
    RTTI_PROPERTY(m_contentType).editable("Content type of the image, usually guessed by the file name postfix").overriddable();
    RTTI_PROPERTY(m_contentColorSpace).editable("Color space of the image's content").overriddable();
    RTTI_PROPERTY(m_contentAlphaMode).editable("Alpha mode of the image").overriddable();
    RTTI_CATEGORY("Mipmapping");
    RTTI_PROPERTY(m_mipmapMode).editable("Mipmap generation mode").overriddable();
    RTTI_CATEGORY("Compression");
    RTTI_PROPERTY(m_compressionMode).editable("Compression mode").overriddable();
    RTTI_PROPERTY(m_compressionMasking).editable("Masking mode for determining pixels taking part in compression").overriddable();
    RTTI_PROPERTY(m_compressionQuality).editable("Compression quality").overriddable();
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
    RTTI_OLD_NAME("rendering::StaticTextureFromImageImporter");
    RTTI_METADATA(ResourceImportedClassMetadata).addClass<StaticTexture2D>();
    RTTI_METADATA(ResourceSourceFormatMetadata).addSourceExtensions("bmp;dds;png;jpg;jpeg;jp2;jpx;tga;tif;tiff;hdr;exr;ppm;pbm;psd;xbm;nef;xpm;gif;webp");
    RTTI_METADATA(ResourceImporterConfigurationClassMetadata).configurationClass<StaticTextureCompressionConfiguration>();
RTTI_END_TYPE();

StaticTextureFromImageImporter::StaticTextureFromImageImporter()
{}

static StringView GetTextureSuffix(StringView path)
{
    auto fileName = path.afterLastOrFull("/").beforeFirstOrFull(".");
    if (fileName.empty())
        return StringBuf::EMPTY();

    return fileName.afterLast("_");
}

ResourcePtr StaticTextureFromImageImporter::importResource(IResourceImporterInterface& importer) const
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
    auto imageData = image::LoadImageWithFreeImage(bufferData.data(), bufferData.size());
    if (!imageData)
    {
        TRACE_ERROR("Unable to load image from '{}'", importer.queryImportPath());
        return nullptr;
    }

    // get compression configuration
    auto config = importer.queryConfigration<StaticTextureCompressionConfiguration>();

    // get settings 
    auto settings = config->loadSettings();
    settings.m_suffix = StringBuf(GetTextureSuffix(importPath));

    // TODO: reuse compressed data if for some reason the format is the same

    // compile the image
    auto compressedData = CompressImage(imageData->view(), settings, importer);
    if (!compressedData)
    {
        TRACE_ERROR("Image compression failed for from '{}'", importPath);
        return nullptr;
    }

    // TODO: split data into streamable/persistent part
    IStaticTexture::Setup setup;
    setup.data = std::move(compressedData->data);
    setup.mips = std::move(compressedData->mips);
    setup.info = compressedData->info;

    return RefNew<StaticTexture2D>(std::move(setup));
}

//---
    
END_BOOMER_NAMESPACE_EX(assets)
