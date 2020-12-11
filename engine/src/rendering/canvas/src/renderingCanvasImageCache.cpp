/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "renderingCanvasImageCache.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"
#include "base/system/include/scopeLock.h"

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

        CanvasImageCache::CanvasImageCache(IDevice* dev, ImageFormat imageFormat, uint32_t size, uint32_t pageCount, uint32_t maxEntries)
            : m_invAtlasWidth(1.0f / size)
			, m_invAtlasHeight(1.0f / size)
        {
			// space for image entries is limited by the size of the constant buffer
			m_maxEntries = std::min<uint32_t>(65536 / sizeof(GPUCanvasImageInfo), maxEntries);

			// create image
			{
				ImageCreationInfo info;
				info.allowCopies = true;
				info.allowDynamicUpdate = true;
				info.allowShaderReads = true;
				info.width = size;
				info.height = size;
				info.numSlices = pageCount;
				info.format = imageFormat;
				info.view = ImageViewType::View2DArray;
				info.label = "CanvasImageAtlas";
				m_atlasImage = dev->createImage(info);
				m_atlasImageSRV = m_atlasImage->createSampledView();
			}

			// create image entry buffer
			{
				BufferCreationInfo info;
				info.allowCopies = true;
				info.allowDynamicUpdate = true;
				info.allowShaderReads = true;
				info.stride = sizeof(GPUCanvasImageInfo);
				info.size = m_maxEntries * info.stride;
				info.label = "CanvasImageEntries";
				m_atlasInfoBuffer = dev->createBuffer(info);
				m_atlasInfoBufferSRV = m_atlasInfoBuffer->createStructuredView();
			}

			// initialize pages
			m_pages.reserve(pageCount);
			for (uint32_t i = 0; i < pageCount; ++i)
			{
				auto& page = m_pages.emplaceBack(size);
				page.dirtyRect = base::Rect();
			}

			// reset dirty range
			m_dirtyEntryInfoMin = m_maxEntries;
			m_dirtyEntryInfoMax = 0;
        }

        CanvasImageCache::~CanvasImageCache()
        {
			m_atlasImage.reset();
			m_atlasInfoBuffer.reset();
        }

		const base::canvas::ImageAtlasEntryInfo* CanvasImageCache::fetchImageData(uint32_t entryIndex) const
		{
			if (entryIndex < m_entries.size())
			{
				const auto& entry = m_entries[entryIndex];
				if (entry.placement.pageIndex >= 0)
					return &entry.placement;
			}

			return nullptr;
		}

		bool CanvasImageCache::allocateEntryIndex(uint32_t& outIndex)
		{
			if (!m_freeEntryIndices.empty())
			{
				outIndex = m_freeEntryIndices.back();
				m_freeEntryIndices.popBack();
				return true;
			}

			if (m_entries.size() < m_maxEntries)
			{
				outIndex = m_entries.size();
				m_entries.emplaceBack();
				return true;
			}

			return false;
		}

		base::canvas::ImageEntry CanvasImageCache::registerImage(const base::image::Image* ptr, bool supportWrapping, int additionalPixelBorder)
		{
			DEBUG_CHECK_RETURN_EX_V(ptr, "Invalid image", base::canvas::ImageEntry());

			base::canvas::ImageEntry ret;

			uint32_t index = 0;
			if (allocateEntryIndex(index))
			{
				// return persistent entry data - this stays even if we fail to allocate texture
				ret.atlasIndex = 1; // filled by the caller
				ret.entryIndex = index;
				ret.width = ptr->width();
				ret.height = ptr->height();

				// setup entry source data
				auto& entry = m_entries[index];
				entry.data = AddRef(ptr);
				entry.additionalPixelBorder = additionalPixelBorder;
				entry.supportWrapping = supportWrapping;
				entry.placement = base::canvas::ImageAtlasEntryInfo();

				// convert to 4 channels
				if (entry.data->channels() != 4)
					entry.data = base::image::ConvertChannels(entry.data->view(), 4, &base::Color::WHITE);

				// add entry to dirty range so it's sent to GPU
				m_dirtyEntryInfoMin = std::min<uint16_t>(m_dirtyEntryInfoMin, index);
				m_dirtyEntryInfoMax = std::max<uint16_t>(m_dirtyEntryInfoMax, index+1);

				// try to place entry in the textures
				placeImage(entry, entry.placement);
			}

			return ret;
		}

		void CanvasImageCache::unregisterImage(base::canvas::ImageEntry entry)
		{
			DEBUG_CHECK_RETURN_EX(entry.entryIndex < m_entries.size(), "Invalid entry index");

			auto* info = m_entries.typedData() + entry.entryIndex;
			DEBUG_CHECK_RETURN_EX(info->data, "Entry is not allocated");
			DEBUG_CHECK_RETURN_EX(info->data->width() == entry.width, "Entry image size does not match");
			DEBUG_CHECK_RETURN_EX(info->data->height() == entry.height, "Entry image size does not match");

			info->data.reset();
			memzero(info, sizeof(Entry));
		}        

        //---

		CanvasImageCache::Page::Page(uint32_t size)
			: image(size, size, 4)
		{}

		bool CanvasImageCache::placeImage(const Entry& source, base::canvas::ImageAtlasEntryInfo& outPlacement)
		{
			// compute size requirements
			int padding = 2;
			padding += source.supportWrapping ? 2 : 0;
			padding += source.additionalPixelBorder;

			// align tile sizes to 4 pixels
			const int requiredWidth = base::Align<uint32_t>(source.data->width() + padding, 4);
			const int requiredHeight = base::Align<uint32_t>(source.data->height() + padding, 4);

			// try to allocate in existing space
			for (auto pageIndex : m_pages.indexRange())
			{
				auto& page = m_pages[pageIndex];

				// place the image
				base::image::DynamicAtlasEntry placedImage;
				if (page.image.placeImage(source.data->view(), padding, source.supportWrapping, placedImage))
				{
					page.dirtyRect.merge(placedImage.dirtyRect); // remember what region of GPU texture to update

					outPlacement.pageIndex = pageIndex;
					outPlacement.uvOffset = placedImage.uvStart;
					outPlacement.uvMax = placedImage.uvEnd;
					outPlacement.uvScale.x = (placedImage.uvEnd.x - placedImage.uvStart.x) / source.data->width();
					outPlacement.uvScale.y = (placedImage.uvEnd.y - placedImage.uvStart.y) / source.data->height();
					return true;
				}
			}

			// out of texture space
			return false;			
		}

		//--

		void CanvasImageCache::flushUpdate(command::CommandWriter& cmd)
		{
			// flush buffer entries
			if (m_dirtyEntryInfoMax > m_dirtyEntryInfoMin)
			{
				cmd.opTransitionLayout(m_atlasInfoBuffer, ResourceLayout::ShaderResource, ResourceLayout::CopyDest);

				const auto writeOffset = m_dirtyEntryInfoMin * sizeof(GPUCanvasImageInfo);
				const auto writeSize = (m_dirtyEntryInfoMax - m_dirtyEntryInfoMin) * sizeof(GPUCanvasImageInfo);
				auto* writePtr = cmd.opUpdateDynamicBufferPtrN<GPUCanvasImageInfo>(m_atlasInfoBuffer, writeOffset, writeSize);
				const auto* readPtr = m_entries.typedData() + m_dirtyEntryInfoMin;
				const auto* readEndPtr = m_entries.typedData() + m_dirtyEntryInfoMax;

				while (readPtr < readEndPtr)
				{
					writePtr->uvMin = readPtr->placement.uvOffset;
					writePtr->uvMax = readPtr->placement.uvMax;
					writePtr->uvScale = readPtr->placement.uvMax - readPtr->placement.uvOffset;
					writePtr->uvInvScale.x = (writePtr->uvScale.x > 0.0f) ? 1.0f / writePtr->uvScale.x : 0.0f;
					writePtr->uvInvScale.y = (writePtr->uvScale.y > 0.0f) ? 1.0f / writePtr->uvScale.y : 0.0f;

					++readPtr;
					++writePtr;
				}

				m_dirtyEntryInfoMin = m_maxEntries;
				m_dirtyEntryInfoMax = 0;

				cmd.opTransitionLayout(m_atlasInfoBuffer, ResourceLayout::CopyDest, ResourceLayout::ShaderResource);

			}

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