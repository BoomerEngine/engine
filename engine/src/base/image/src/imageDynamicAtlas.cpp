/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: image\atlas #]
***/

#include "build.h"

#include "image.h"
#include "imageDynamicAtlas.h"
#include "imageUtils.h"
#include "imageView.h"

#include "base/containers/include/rectAllocator.h"

namespace base
{
    namespace image
    {

        DynamicAtlas::DynamicAtlas(uint32_t width, uint32_t height, uint8_t numChannels /*= 1*/, uint32_t pixelSpacing /*= 1*/, PixelFormat format /*= PixelFormat::Uint8_Norm*/)
            : m_width(width)
            , m_height(height)
            , m_invWidth(1.0f / (float)width)
            , m_invHeight(1.0f / (float)height)
            , m_pixelSpacing(pixelSpacing)
        {
            // create the backing image
            m_image = CreateSharedPtr<Image>(format, numChannels, width, height);

            // create the space allocator
            m_spaceAllocator = CreateUniquePtr<RectAllocator>();
            m_spaceAllocator->reset(m_width, m_height);
        }

        DynamicAtlas::~DynamicAtlas()
        {}

        void DynamicAtlas::reset()
        {
            Fill(m_image->view(), &base::Color::BLACK);
            m_spaceAllocator->reset(m_width, m_height);
        }

        bool DynamicAtlas::placeImage(const ImageView& sourceImage, DynamicAtlasEntry& outEntry)
        {
            // determine the size of the pixel area to allocate
            auto requiredWidth = m_pixelSpacing * 2 + sourceImage.width();
            auto requiredHeight = m_pixelSpacing * 2 + sourceImage.height();

            // allocate rect in the image using the space allocator
            uint32_t offsetX = 0, offsetY = 0;
            if (!m_spaceAllocator->allocate(requiredWidth, requiredHeight, offsetX, offsetY))
                return false; // out of space

            // create entry
            outEntry.placement.min.x = offsetX + m_pixelSpacing;
            outEntry.placement.min.y = offsetY + m_pixelSpacing;
            outEntry.placement.max.x = outEntry.placement.min.x + sourceImage.width();
            outEntry.placement.max.y = outEntry.placement.min.y + sourceImage.height();
            outEntry.uvStart.x = (float)outEntry.placement.min.x * m_invWidth;
            outEntry.uvStart.y = (float)outEntry.placement.min.y * m_invHeight;
            outEntry.uvEnd.x = (float)outEntry.placement.max.x * m_invWidth;
            outEntry.uvEnd.y = (float)outEntry.placement.max.y * m_invHeight;

            // fill background
            auto fullView = m_image->view().subView(offsetX, offsetY, requiredWidth, requiredHeight);
            Fill(fullView, &Color::BLACK);

            // copy image data
            auto targetView = m_image->view().subView(outEntry.placement.min.x, outEntry.placement.min.y, outEntry.placement.width(), outEntry.placement.height());
            Copy(sourceImage, targetView);

            // advance image version to get it re uploaded if necessary 
            m_image->markModified();
            return true;
        }

    } // image
} // base

