/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#pragma once

#include "base/resources/include/resource.h"

namespace base
{
    namespace image
    {
        /// General image wrapper
        class BASE_IMAGE_API Image : public res::IResource
        {
            RTTI_DECLARE_VIRTUAL_CLASS(Image, res::IResource);

        public:
            Image();
            Image(const ImageView& copyFrom); // copy of image

            // empty image (unitialized DATA - random bytes)
            Image(PixelFormat format, uint8_t channels, uint32_t width); // 1D
            Image(PixelFormat format, uint8_t channels, uint32_t width, uint32_t height); // 2D
            Image(PixelFormat format, uint8_t channels, uint32_t width, uint32_t height, uint32_t depth); // 3D

            // filled image (data intialized to copy of the provided fill value that depends on the pixel format)
            Image(PixelFormat format, uint8_t channels, uint32_t width, const void* clearValue); // 1D
            Image(PixelFormat format, uint8_t channels, uint32_t width, uint32_t height, const void* clearValue); // 2D
            Image(PixelFormat format, uint8_t channels, uint32_t width, uint32_t height, uint32_t depth, const void* clearValue); // 3D

            // simple 2D color filled image (most common case)
            Image(uint32_t width, uint32_t height, base::Color color);
            
            //--

            INLINE PixelFormat format() const { return m_format; }

            INLINE uint8_t channels() const { return m_channels; }

            INLINE uint32_t width() const { return m_width; }
            INLINE uint32_t height() const { return m_height; }
            INLINE uint32_t depth() const { return m_depth; }

            INLINE float invWidth() const { return m_invWidth; }
            INLINE float invHeight() const { return m_invHeight; }
            INLINE float invDepth() const { return m_invDepth; }

            INLINE uint32_t pixelPitch() const { return m_pixelPitch; }
            INLINE uint32_t rowPitch() const { return m_rowPitch; }
            INLINE uint32_t slicePitch() const { return m_slicePitch; }

            INLINE const uint8_t* data() const { return m_pixels.data(); }
            INLINE uint8_t* data() { return m_pixels.data(); }

            INLINE const Buffer& buffer() const { return m_pixels; } // NOTE: it will keep changing as the image changes

            //--

            // Get valid view of the image
            ImageView view();

            // Get valid view of the image
            ImageView view() const;

            //--

        private:
            Buffer m_pixels;

            PixelFormat m_format = PixelFormat::Uint8_Norm;

            uint8_t m_channels = 0;
            uint32_t m_width = 0;
            uint32_t m_height = 0;
            uint32_t m_depth = 0;

            float m_invWidth = 1.0f;
            float m_invHeight = 1.0f;
            float m_invDepth = 1.0f;

            uint32_t m_pixelPitch = 0;
            uint32_t m_rowPitch = 0;
            uint32_t m_slicePitch = 0;

            void allocPixels(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, uint8_t channels);
            void freePixels();

            virtual bool supportsReloading() const { return true; };
            virtual void onPostLoad() override;
        };

    } // image
} // base