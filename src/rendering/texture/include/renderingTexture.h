/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: texture #]
***/

#pragma once

#include "base/resource/include/resource.h"

namespace rendering
{
    //---

    enum class TextureFilteringMode : uint8_t
    {
        Point, // point filtering
        Bilinear, // filter texels only, hash switch between mipmaps
        Trilinear, // filter texels + mipmaps (default filtering mode)
        Aniso, // use anisotropy filtering for textures in this group, only if anisotropy is enabled
    };

    // general texture information
    struct RENDERING_TEXTURE_API TextureInfo
    {
        RTTI_DECLARE_NONVIRTUAL_CLASS(TextureInfo);

        // color space of the data in the texture, usually linear or sRGB
        base::image::ColorSpace colorSpace = base::image::ColorSpace::Linear;

        // texture size information
        ImageViewType type = ImageViewType::View2D; // resource type
        ImageFormat format = ImageFormat::UNKNOWN; // rendering format
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 0; // 3D textures only
        uint32_t slices = 0; // arrays only
        uint32_t mips = 0;

        // filtering
        TextureFilteringMode filterMode = TextureFilteringMode::Aniso;
        uint8_t filterMaxAniso = 0;

        // flags
        bool compressed = false; // texture is in compressed format
        bool premultipliedAlpha = false; // texture's color was premutliplied with alpha
    };
        
    //---

    /// abstract texture to be used in the rendering pipeline
    class RENDERING_TEXTURE_API ITexture : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ITexture, base::res::IResource);

    public:
        ITexture(const TextureInfo& info);
        virtual ~ITexture();

        //--

        // get texture info
        inline const TextureInfo& info() const { return m_info; }

        //--

        // resolve texture to a image view that can be used in the rendering
        virtual ImageSampledViewPtr view() const = 0;

    protected:
        TextureInfo m_info;
    };

    //---

} // rendering