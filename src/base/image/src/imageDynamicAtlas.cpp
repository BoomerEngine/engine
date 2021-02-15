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

		//--

        DynamicAtlas::DynamicAtlas(uint32_t width, uint32_t height, uint8_t numChannels /*= 1*/, PixelFormat format /*= PixelFormat::Uint8_Norm*/)
            : m_width(width)
            , m_height(height)
            , m_invWidth(1.0f / (float)width)
            , m_invHeight(1.0f / (float)height)
        {
            // create the backing image
            m_image = RefNew<Image>(format, numChannels, width, height);

            // create the space allocator
            m_spaceAllocator = CreateUniquePtr<RectAllocator>();
            m_spaceAllocator->reset(m_width, m_height);
        }

        DynamicAtlas::~DynamicAtlas()
        {}

        void DynamicAtlas::reset()
        {
			const uint32_t zero = 0;
            Fill(m_image->view(), &zero);
            m_spaceAllocator->reset(m_width, m_height);
        }

        bool DynamicAtlas::placeImage(const ImageView& sourceImage, uint32_t padding, bool supportWrapping, DynamicAtlasEntry& outEntry)
        {
            // determine the size of the pixel area to allocate
            auto requiredWidth = padding * 2 + sourceImage.width();
            auto requiredHeight = padding * 2 + sourceImage.height();

            // allocate rect in the image using the space allocator
            uint32_t offsetX = 0, offsetY = 0;
            if (!m_spaceAllocator->allocate(requiredWidth, requiredHeight, offsetX, offsetY))
                return false; // out of space

            // create entry
			outEntry.dirtyRect.min.x = offsetX;
			outEntry.dirtyRect.min.y = offsetY;
			outEntry.dirtyRect.max.x = offsetX + requiredWidth;
			outEntry.dirtyRect.max.y = offsetY + requiredHeight;
			outEntry.placement.min.x = offsetX + padding;
            outEntry.placement.min.y = offsetY + padding;
            outEntry.placement.max.x = outEntry.placement.min.x + sourceImage.width();
            outEntry.placement.max.y = outEntry.placement.min.y + sourceImage.height();
            outEntry.uvStart.x = (float)outEntry.placement.min.x * m_invWidth;
            outEntry.uvStart.y = (float)outEntry.placement.min.y * m_invHeight;
            outEntry.uvEnd.x = (float)outEntry.placement.max.x * m_invWidth;
            outEntry.uvEnd.y = (float)outEntry.placement.max.y * m_invHeight;

            // fill background
            auto fullView = m_image->view().subView(offsetX, offsetY, requiredWidth, requiredHeight);
			const uint32_t zero = 0;
            Fill(fullView, &zero);

			// determine where to copy the image to
			auto targetView = m_image->view().subView(outEntry.placement.min.x, outEntry.placement.min.y, 
				outEntry.placement.width(), outEntry.placement.height());
				
			// copy original image 
			Copy(sourceImage, targetView);

			// if we want to support wrapping we neet to manually repeat some part of the image around the tile
			// this is to prevent texture interpolation issues
			if (supportWrapping)
			{
				const auto p = padding;
				const auto w = sourceImage.width();
				const auto h = sourceImage.height();

				// we can only copy as must as we have source data
				const auto mx = std::min<uint32_t>(p, w);
				const auto my = std::min<uint32_t>(p, h);

				// top/bottom line
				base::image::Copy(sourceImage.subView(0, 0, w, my), fullView.subView(p, p+h, w, my));
				base::image::Copy(sourceImage.subView(0, h - my, w, my), fullView.subView(p, p-my, w, my));

				// left/right line
				base::image::Copy(sourceImage.subView(0, 0, mx, h), fullView.subView(p+w, p, mx, h));
				base::image::Copy(sourceImage.subView(w - mx, 0, mx, h), fullView.subView(p-mx, p, mx, h));

				// corners
				//base::image::Copy(sourceImage.subView(0, 0, m, m), ret->view().subView(fw - m, fh - m, m, m));
				//base::image::Copy(sourceImage.subView(w - m, h - m, m, m), ret->view().subView(0, 0, m, m));
				//base::image::Copy(sourceImage.subView(0, h - m, m, m), ret->view().subView(fw - m, 0, m, m));
				//base::image::Copy(sourceImage.subView(w - m, 0, m, m), ret->view().subView(0, fh - m, m, m));
			}

            // advance image version to get it re uploaded if necessary 
            m_image->markModified();
            return true;
        }

		//--

    } // image
} // base

