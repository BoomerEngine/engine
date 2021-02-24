/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "imageCompression.h"
#include "base/resource_compiler/include/importInterface.h"
#include "base/resource/include/resourceMetadata.h"

BEGIN_BOOMER_NAMESPACE(assets)

//--

/// texture compression configuration
class IMPORT_TEXTURE_LOADER_API StaticTextureCompressionConfiguration : public base::res::ResourceConfiguration
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureCompressionConfiguration, base::res::ResourceConfiguration);

public:
    StaticTextureCompressionConfiguration();

    ImageContentType m_contentType = ImageContentType::Auto;
    ImageContentColorSpace m_contentColorSpace = ImageContentColorSpace::Auto;
    ImageAlphaMode m_contentAlphaMode = ImageAlphaMode::NotPremultiplied;

    ImageMipmapGenerationMode m_mipmapMode = ImageMipmapGenerationMode::Auto;

    ImageCompressionFormat m_compressionMode = ImageCompressionFormat::Auto;
    ImageValidPixelsMaskingMode m_compressionMasking = ImageValidPixelsMaskingMode::Auto;
    ImageCompressionQuality m_compressionQuality = ImageCompressionQuality::Normal;

    virtual void computeConfigurationKey(base::CRC64& crc) const override;

    ImageCompressionSettings loadSettings() const;
};

//--

// importer for static textures from images
class IMPORT_TEXTURE_LOADER_API StaticTextureFromImageImporter : public base::res::IResourceImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureFromImageImporter, base::res::IResourceImporter);

public:
    StaticTextureFromImageImporter();

    virtual base::res::ResourcePtr importResource(base::res::IResourceImporterInterface& importer) const override final;
};

//--

END_BOOMER_NAMESPACE(assets)