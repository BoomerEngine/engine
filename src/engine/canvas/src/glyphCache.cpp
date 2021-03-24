/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "canvas.h"
#include "glyphCache.h"
#include "service.h"

#include "core/image/include/image.h"
#include "core/image/include/imageView.h"
#include "core/image/include/imageUtils.h"
#include "engine/font/include/fontGlyph.h"

#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/resources.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/image.h"
#include "gpu/device/include/buffer.h"

BEGIN_BOOMER_NAMESPACE()

//--

CanvasGlyphCache::CanvasGlyphCache(uint32_t size, uint32_t maxPages)
{
	// image size
	m_invAtlasSize = 1.0f / size;
	m_texelOffset = 0.5f / size;

	// initialize pages
	m_pages.reserve(maxPages);
	for (uint32_t i = 0; i < maxPages; ++i)
	{
		auto& page = m_pages.emplaceBack(size);
		page.dirtyRect = Rect();
	}

	// initialize entries
	m_entriesMap.reserve(4096);

    //--

    auto* dev = GetService<DeviceService>()->device();

    // create image
    {
        gpu::ImageCreationInfo info;
        info.allowCopies = true;
        info.allowDynamicUpdate = true;
        info.allowShaderReads = true;
        info.width = size;
        info.height = size;
        info.numSlices = maxPages;
        info.format = ImageFormat::R8_UNORM;// SRGBA8;
        info.view = ImageViewType::View2DArray;
        info.label = "CanvasGlyphAtlas";
        m_image = dev->createImage(info);
        m_imageSRV = m_image->createSampledView();
    }

    // create image entry buffer
    /*{
        gpu::BufferCreationInfo info;
        info.allowCopies = true;
        info.allowDynamicUpdate = true;
        info.allowShaderReads = true;
        info.stride = sizeof(GPUCanvasImageInfo);
        info.size = 2048 * info.stride;
        info.label = "CanvasGlyphEntries";
        m_infoBuffer = dev->createBuffer(info);
        m_infoBufferSRV = m_infoBuffer->createStructuredView();
    }*/
}

CanvasGlyphCache::~CanvasGlyphCache()
{
}

CanvasGlyphCache::Page::Page(uint32_t size)
	: image(size, size, 1)
{}

const CanvasImageEntryInfo* CanvasGlyphCache::findRenderDataForGlyph(const font::Glyph* glyph)
{
	DEBUG_CHECK_RETURN_EX_V(glyph, "Invalid input glyph", nullptr);

	// find in cache
	auto& entryInfo = m_entriesMap[glyph->id()];

	// if not placed place it now
	if (entryInfo.placement.pageIndex < 0)
	{
		entryInfo.glyph = glyph;

		// try to allocate in existing space
		for (auto pageIndex : m_pages.indexRange())
		{
			auto& page = m_pages[pageIndex];

			// place the image
			ImageAtlasEntry placedImage;
			if (page.image.placeImage(glyph->bitmap()->view(), 4, false, placedImage))
			{
				// remember placement
				entryInfo.placement.pageIndex = pageIndex;
				entryInfo.placement.wrap = false;
				entryInfo.placement.uvOffset = placedImage.uvStart;
				entryInfo.placement.uvMax = placedImage.uvEnd;
				entryInfo.placement.uvScale = placedImage.uvEnd - placedImage.uvStart;

				// apply half-texel offset to compensate for platforms
				/*entryInfo.placement.uvOffset.x -= m_texelOffset;
				entryInfo.placement.uvOffset.y -= m_texelOffset;
				entryInfo.placement.uvMax.x -= m_texelOffset;
				entryInfo.placement.uvMax.y -= m_texelOffset;*/

				// remember what region of GPU texture to update
				page.dirtyRect.merge(placedImage.dirtyRect);

				// mark as dirty
				m_dirty = true;
			}
		}
	}

	return &entryInfo.placement;
}

//--

void CanvasGlyphCache::flush(gpu::CommandWriter& cmd, uint64_t pageMask)
{
	if (!m_dirty)
		return;

    for (auto pageIndex : m_pages.indexRange())
    {
        auto& page = m_pages[pageIndex];
        if (!page.dirtyRect.empty())
        {
            const auto dirtySubImageView = page.image.image()->view().subView(
                page.dirtyRect.min.x, page.dirtyRect.min.y,
                page.dirtyRect.width(), page.dirtyRect.height());

            cmd.opTransitionLayout(m_image, gpu::ResourceLayout::ShaderResource, gpu::ResourceLayout::CopyDest);
            cmd.opUpdateDynamicImage(m_image, dirtySubImageView, 0, pageIndex, page.dirtyRect.min.x, page.dirtyRect.min.y);
            cmd.opTransitionLayout(m_image, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::ShaderResource);
        }
    }

    m_dirty = false;
}

//--
        
END_BOOMER_NAMESPACE()
