/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#include "build.h"
#include "image.h"
#include "imageView.h"
#include "imageUtils.h"
#include "base/resource/include/resourceTags.h"

namespace base
{
    namespace image
    {

        //--

        RTTI_BEGIN_TYPE_ENUM(PixelFormat);
            RTTI_ENUM_OPTION(Uint8_Norm);
            RTTI_ENUM_OPTION(Uint16_Norm);
            RTTI_ENUM_OPTION(Float16_Raw);
            RTTI_ENUM_OPTION(Float32_Raw);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_ENUM(ColorSpace);
            RTTI_ENUM_OPTION(Linear);
            RTTI_ENUM_OPTION(SRGB);
            RTTI_ENUM_OPTION(HDR);
            RTTI_ENUM_OPTION(Normals);
        RTTI_END_TYPE();

        RTTI_BEGIN_TYPE_ENUM(CubeSide);
            RTTI_ENUM_OPTION(PositiveX);
            RTTI_ENUM_OPTION(NegativeX);
            RTTI_ENUM_OPTION(PositiveY);
            RTTI_ENUM_OPTION(NegativeY);
            RTTI_ENUM_OPTION(PositiveZ);
            RTTI_ENUM_OPTION(NegativeZ);
        RTTI_END_TYPE();

        //--

        RTTI_BEGIN_TYPE_CLASS(Image);
            RTTI_METADATA(res::ResourceExtensionMetadata).extension("v4bitmap");
            RTTI_METADATA(res::ResourceDescriptionMetadata).description("Bitmap Image");
            RTTI_PROPERTY(m_format);
            RTTI_PROPERTY(m_channels);
            RTTI_PROPERTY(m_width);
            RTTI_PROPERTY(m_height);
            RTTI_PROPERTY(m_depth);
            RTTI_PROPERTY(m_pixelPitch);
            RTTI_PROPERTY(m_rowPitch);
            RTTI_PROPERTY(m_slicePitch);
            RTTI_PROPERTY(m_pixels);
        RTTI_END_TYPE();

        //--

        Image::Image()
        {
        }

        Image::Image(const ImageView& copyFrom)
        {
            allocPixels(copyFrom.width(), copyFrom.height(), copyFrom.depth(), copyFrom.format(), copyFrom.channels());
            Copy(copyFrom, view());
        }

        Image::Image(PixelFormat format, uint8_t channels, uint32_t width)
        {
            allocPixels(width, 1, 1, format, channels);
        }

        Image::Image(PixelFormat format, uint8_t channels, uint32_t width, uint32_t height)
        {
            allocPixels(width, height, 1, format, channels);
        }

        Image::Image(PixelFormat format, uint8_t channels, uint32_t width, uint32_t height, uint32_t depth)
        {
            allocPixels(width, height, depth, format, channels);
        }

        Image::Image(PixelFormat format, uint8_t channels, uint32_t width, const void* fillColor)
        {
            allocPixels(width, 1, 1, format, channels);
            Fill(view(), fillColor);
        }

        Image::Image(PixelFormat format, uint8_t channels, uint32_t width, uint32_t height, const void* fillColor)
        {
            allocPixels(width, height, 1, format, channels);
            Fill(view(), fillColor);
        }

        Image::Image(PixelFormat format, uint8_t channels, uint32_t width, uint32_t height, uint32_t depth, const void* fillColor)
        {
            allocPixels(width, height, depth, format, channels);
            Fill(view(), fillColor);
        }

        Image::Image(uint32_t width, uint32_t height, base::Color color)
        {
            allocPixels(width, height, 1, PixelFormat::Uint8_Norm, 4);
            Fill(view(), &color);
        }

        //--

        void Image::allocPixels(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, uint8_t channels)
        {
            DEBUG_CHECK(width >= 1);
            DEBUG_CHECK(height >= 1);
            DEBUG_CHECK(depth >= 1);
            DEBUG_CHECK(channels >= 1 && channels <= 4);

            if (m_width != width || m_height != height || m_depth != depth || m_format != format || m_channels != channels)
            {
                switch (format)
                {
                    case PixelFormat::Uint8_Norm: m_pixelPitch = channels; break;
                    case PixelFormat::Uint16_Norm: m_pixelPitch = channels * 2; break;
                    case PixelFormat::Float16_Raw: m_pixelPitch = channels * 2; break;
                    case PixelFormat::Float32_Raw: m_pixelPitch = channels * 4; break;
                }

                m_width = width;
                m_height = height;
                m_depth = depth;
                m_format = format;
                m_channels = channels;
                m_rowPitch = m_pixelPitch * m_width;
                m_slicePitch = m_rowPitch * m_height;
                m_invWidth = 1.0f / m_width;
                m_invHeight = 1.0f / m_height;
                m_invDepth = 1.0f / m_depth;

                auto dataSize = m_slicePitch * m_depth;
                m_pixels = Buffer::Create(POOL_IMAGE, dataSize);
            }
        }

        void Image::freePixels()
        {
            m_pixels.reset();
        }

        ImageView Image::view()
        {
            if (m_depth >= 1)
                return ImageView(m_format, m_channels, m_pixels.data(), m_width, m_height, m_depth, m_pixelPitch, m_rowPitch, m_slicePitch);
            else if (m_height >= 1)
                return ImageView(m_format, m_channels, m_pixels.data(), m_width, m_height, m_pixelPitch, m_rowPitch);
            else if (m_width >= 1)
                return ImageView(m_format, m_channels, m_pixels.data(), m_width, m_pixelPitch);
            else
                return ImageView();
        }

        ImageView Image::view() const
        {
            if (m_depth >= 1)
                return ImageView(m_format, m_channels, m_pixels.data(), m_width, m_height, m_depth, m_pixelPitch, m_rowPitch, m_slicePitch);
            else if (m_height >= 1)
                return ImageView(m_format, m_channels, m_pixels.data(), m_width, m_height, m_pixelPitch, m_rowPitch);
            else if (m_width >= 1)
                return ImageView(m_format, m_channels, m_pixels.data(), m_width, m_pixelPitch);
            else
                return ImageView();
        }

        void Image::onPostLoad()
        {
            TBaseClass::onPostLoad();
            m_invWidth = 1.0f / m_width;
            m_invHeight = 1.0f / m_height;
            m_invDepth = 1.0f / m_depth;
        }

        /*void Image::applyReload(const IResource& reloadedData)
        {
            auto& src = (Image&)reloadedData;

            freePixels();

            m_pixels = std::move(src.m_pixels);
            m_format = src.m_format;
            m_channels = src.m_channels;
            m_width = src.m_width;
            m_height = src.m_height;
            m_depth = src.m_depth;
            m_pixelPitch = src.m_pixelPitch;
            m_rowPitch = src.m_rowPitch;
            m_slicePitch = src.m_slicePitch;
            m_invWidth = 1.0f / m_width;
            m_invHeight = 1.0f / m_height;
            m_invDepth = 1.0f / m_depth;

            src.m_channels = 0;
            src.m_width = 0;
            src.m_height = 0;
            src.m_depth = 0;
            src.m_pixelPitch = 0;
            src.m_rowPitch = 0;
            src.m_slicePitch = 0;

            invalidateRuntimeVersion();

            TBaseClass::applyReload(reloadedData);
        }*/

        //--

    } // base
} // image