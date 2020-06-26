/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image #]
***/

#include "build.h"
#include "imageView.h"
#include "imageUtils.h"

namespace base
{
    namespace image
    {

        //--

        ImageView ImageView::subView(uint32_t x, uint32_t width) const
        {
            ASSERT(x + width <= m_width);
            return ImageView(m_format, m_channels, pixelPtr(x), width, m_pixelPitch);
        }

        ImageView ImageView::subView(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const
        {
            ASSERT(x + width <= m_width);
            ASSERT(y + height <= m_height);
            return ImageView(m_format, m_channels, pixelPtr(x, y), width, height, m_pixelPitch, m_rowPitch);
        }

        ImageView ImageView::subView(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height, uint32_t depth) const
        {
            ASSERT(x + width <= m_width);
            ASSERT(y + height <= m_height);
            ASSERT(z + depth <= m_depth);
            return ImageView(m_format, m_channels, pixelPtr(x, y, z), width, height, depth, m_pixelPitch, m_rowPitch, m_slicePitch);
        }

        //--

        Buffer ImageView::toBuffer(mem::PoolID id) const
        {
            if (empty())
                return nullptr;

            auto size = dataSize();

            auto buffer = Buffer::Create(id, size);
            if (!buffer)
                return nullptr;

            ImageView packedView(NATIVE_LAYOUT, format(), channels(), buffer.data(), width(), height(), depth());
            Copy(*this, packedView);

            return buffer;
        }

        void ImageView::copy(void* memPtr) const
        {
            if (!empty())
            {
                ImageView packedView(NATIVE_LAYOUT, format(), channels(), memPtr, width(), height(), depth());
                Copy(*this, packedView);
            }
        }

        //--

    } // image
} // base