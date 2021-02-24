/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "canvas.h"
#include "canvasGlyphCache.h"
#include "canvasService.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"
#include "base/font/include/fontGlyph.h"

BEGIN_BOOMER_NAMESPACE(base::canvas)

//--

GlyphCache::GlyphCache(uint32_t size, uint32_t maxPages)
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
}

GlyphCache::~GlyphCache()
{
}

GlyphCache::Page::Page(uint32_t size)
	: image(size, size, 1)
{}

const canvas::ImageAtlasEntryInfo* GlyphCache::findRenderDataForGlyph(const font::Glyph* glyph)
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
			image::DynamicAtlasEntry placedImage;
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

void GlyphCache::flush(rendering::GPUCommandWriter& cmd, uint64_t pageMask, ICanvasAtlasSync& sync)
{
	if (!m_dirty)
		return;

	ICanvasAtlasSync::AtlasUpdate update;
	update.numPages = m_pages.size();
	update.size = m_pages[0].image.width();

	for (auto pageIndex : m_pages.indexRange())
	{
		auto& page = m_pages[pageIndex];
		if (!page.dirtyRect.empty())
		{
			auto& pageUpdate = update.pageUpdate.emplaceBack();
			pageUpdate.pageIndex = pageIndex;
			pageUpdate.data = page.image.image()->view();
			pageUpdate.rect = page.dirtyRect;

			page.dirtyRect = Rect();
		}
	}

	sync.updateGlyphCache(cmd, update);

	m_dirty = false;
}

//--
        
END_BOOMER_NAMESPACE(base::canvas)
