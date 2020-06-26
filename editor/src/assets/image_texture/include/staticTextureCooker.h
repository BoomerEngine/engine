/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "imageCompression.h"

namespace rendering
{
    //--

    /// manifest for static texture baking
    class ASSETS_IMAGE_TEXTURE_API StaticTextureFromImageManifest : public base::res::IResourceManifest
    {
        RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureFromImageManifest, base::res::IResourceManifest);

    public:
        StaticTextureFromImageManifest();

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

    // cooker for static texture
    class ASSETS_IMAGE_TEXTURE_API StaticTextureFromImageCooker : public base::res::IResourceCooker
    {
        RTTI_DECLARE_VIRTUAL_CLASS(StaticTextureFromImageCooker, base::res::IResourceCooker);

    public:
        StaticTextureFromImageCooker();

        virtual void reportManifestClasses(base::Array<base::SpecificClassType<base::res::IResourceManifest>>& outManifestClasses) const override final;
        virtual base::res::ResourceHandle cook(base::res::IResourceCookerInterface& cooker) const override final;
    };

    //--

} // rendering