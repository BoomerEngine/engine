/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasGlyphCache.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"
#include "base/font/include/fontGlyph.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingObject.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingResources.h"

namespace rendering
{
    namespace canvas
    {
		//---

		base::ConfigProperty<uint32_t> cvCanvasGlyphCacheAtlasSize("Rendering.Canvas", "GlyphCacheAtlasSize", 2048);
		base::ConfigProperty<uint32_t> cvCanvasGlyphCacheAtlasPageCount("Rendering.Canvas", "GlyphCacheAtlasPageCount", 1);

        //---

        CanvasGlyphCache::CanvasGlyphCache(IDevice* dev)
        {
			// image size
			const auto atlasSize = base::NextPow2(std::clamp<uint32_t>(cvCanvasGlyphCacheAtlasSize.get(), 256, 4096));
			const auto atlasPages = std::clamp<uint32_t>(cvCanvasGlyphCacheAtlasPageCount.get(), 1, 256);
			m_invAtlasSize = 1.0f / atlasSize;
			m_texelOffset = 0.5f / atlasSize;

			// create image
			{
				ImageCreationInfo info;
				info.allowCopies = true;
				info.allowDynamicUpdate = true;
				info.allowShaderReads = true;
				info.width = atlasSize;
				info.height = atlasSize;
				info.numSlices = atlasPages;
				info.format = ImageFormat::R8_UNORM;
				info.view = ImageViewType::View2DArray;
				info.label = "CanvasGlyphAtlas";
				m_atlasImage = dev->createImage(info);
				m_atlasImageSRV = m_atlasImage->createSampledView();
			}

			// initialize pages
			m_pages.reserve(atlasPages);
			for (uint32_t i = 0; i < atlasPages; ++i)
			{
				auto& page = m_pages.emplaceBack(atlasSize);
				page.dirtyRect = base::Rect();
			}
        }

        CanvasGlyphCache::~CanvasGlyphCache()
        {
			m_atlasImage.reset();
        }

		CanvasGlyphCache::Page::Page(uint32_t size)
			: image(size, size, 1)
		{}

		const base::canvas::ImageAtlasEntryInfo* CanvasGlyphCache::findRenderDataForGlyph(const base::font::Glyph* glyph)
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
					base::image::DynamicAtlasEntry placedImage;
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
					}
				}
			}
			
			return &entryInfo.placement;
		}

		//--

		void CanvasGlyphCache::flushUpdate(command::CommandWriter& cmd)
		{			
			// flush images
			for (auto pageIndex : m_pages.indexRange())
			{
				auto& page = m_pages[pageIndex];
				if (!page.dirtyRect.empty())
				{
					auto dirtySubImage = page.image.image()->view().subView(
						page.dirtyRect.min.x, page.dirtyRect.min.y,
						page.dirtyRect.width(), page.dirtyRect.height());

					cmd.opTransitionLayout(m_atlasImage, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);
					cmd.opUpdateDynamicImage(m_atlasImage, dirtySubImage, 0, pageIndex, page.dirtyRect.min.x, page.dirtyRect.min.y);
					cmd.opTransitionLayout(m_atlasImage, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);

					page.dirtyRect = base::Rect();
				}
			}
		}

		//--

    } // canvas
} // rendering