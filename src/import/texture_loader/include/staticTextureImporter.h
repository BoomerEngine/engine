/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "imageCompression.h"
#include "core/resource_compiler/include/importInterface.h"
#include "core/resource/include/metadata.h"

BEGIN_BOOMER_NAMESPACE_EX(assets)

//--

/// texture compression configuration
class IMPORT_TEXTURE_LOADER_API StaticTextureCompressionConfiguration : public ResourceConfiguration
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureCompressionConfiguration, ResourceConfiguration);

public:
    StaticTextureCompressionConfiguration();

    ImageContentType m_contentType = ImageContentType::Auto;
    ImageContentColorSpace m_contentColorSpace = ImageContentColorSpace::Auto;
    ImageAlphaMode m_contentAlphaMode = ImageAlphaMode::NotPremultiplied;

    ImageMipmapGenerationMode m_mipmapMode = ImageMipmapGenerationMode::Auto;

    ImageCompressionFormat m_compressionMode = ImageCompressionFormat::Auto;
    ImageValidPixelsMaskingMode m_compressionMasking = ImageValidPixelsMaskingMode::Auto;
    ImageCompressionQuality m_compressionQuality = ImageCompressionQuality::Normal;

    ImageCompressionSettings loadSettings() const;
};

//--

// importer for static textures from images
class IMPORT_TEXTURE_LOADER_API StaticTextureFromImageImporter : public IResourceImporter
{
    RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureFromImageImporter, IResourceImporter);

public:
    StaticTextureFromImageImporter();

    virtual ResourcePtr importResource(IResourceImporterInterface& importer) const override final;
};

//--

END_BOOMER_NAMESPACE_EX(assets)
