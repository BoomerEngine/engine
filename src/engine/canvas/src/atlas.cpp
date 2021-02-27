/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"

#include "canvas.h"
#include "geometry.h"
#include "geometryBuilder.h"
#include "batchRenderer.h"
#include "atlas.h"
#include "service.h"

#include "core/image/include/imageUtils.h"
#include "core/image/include/imageView.h"

#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/resources.h"
#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/image.h"
#include "gpu/device/include/buffer.h"

BEGIN_BOOMER_NAMESPACE_EX(canvas)

//---

IAtlas::IAtlas()
{
	if (!IsDefaultObjectCreation())
		GetService<CanvasService>()->registerAtlas(this, m_index);
}

IAtlas::~IAtlas()
{
	GetService<CanvasService>()->unregisterAtlas(this, m_index);
}

//--

ConfigProperty<uint32_t> cvCanvasPageMinSize("Render.Canvas", "AtlasMinSize", 256);
ConfigProperty<uint32_t> cvCanvasPageMaxSize("Render.Canvas", "AtlasMaxSize", 4096);
ConfigProperty<uint32_t> cvCanvasPageSizeGranurality("Render.Canvas", "AtlasSizeGranurality", 64);
ConfigProperty<uint32_t> cvCanvasMaxPages("Rendering.Canvas", "AtlasMaxPages", 64);
ConfigProperty<bool> cvCanvasForcePowerOfTwoPages("Render.Canvas", "ForcePowerOfTwoSizes", false);

DynamicAtlas::DynamicAtlas(uint32_t requestedSize, uint32_t requestedPageCount)
	: m_maxEntries(2048) // TODO: compute
{
	// calculate page size
	const auto pageGranurality = NextPow2(std::clamp<uint32_t>(cvCanvasPageSizeGranurality.get(), 1, 1024));
	const auto pageSize = Align(std::clamp<uint32_t>(requestedSize, cvCanvasPageMinSize.get(), cvCanvasPageMaxSize.get()), pageGranurality);

	// calculate page count
	const auto pageCount = std::clamp<uint32_t>(requestedPageCount, 1, cvCanvasMaxPages.get());

	// calculate final texture size
	const auto textureSize = cvCanvasForcePowerOfTwoPages.get() ? NextPow2(pageSize) : pageSize;

	//--

	m_invAtlasWidth = 1.0f / (float)pageSize;
	m_invAtlasHeight = 1.0f / (float)pageSize;

	// initialize pages
	m_pages.reserve(pageCount);
	for (uint32_t i = 0; i < pageCount; ++i)
	{
		auto& page = m_pages.emplaceBack(pageSize);
		page.dirtyRect = Rect();
	}

	// reset dirty range
	m_dirtyEntryInfoMin = m_maxEntries;
	m_dirtyEntryInfoMax = 0;

	//--

    auto* dev = GetService<DeviceService>()->device();

    // create image
    {
        gpu::ImageCreationInfo info;
        info.allowCopies = true;
        info.allowDynamicUpdate = true;
        info.allowShaderReads = true;
        info.width = textureSize;
        info.height = textureSize;
        info.numSlices = pageCount;
		info.format = ImageFormat::RGBA8_UNORM;// SRGBA8;
        info.view = ImageViewType::View2DArray;
        info.label = "CanvasImageAtlas";
        m_image = dev->createImage(info);
		m_imageSRV = m_image->createSampledView();
    }

    // create image entry buffer
    {
        gpu::BufferCreationInfo info;
        info.allowCopies = true;
        info.allowDynamicUpdate = true;
        info.allowShaderReads = true;
        info.stride = sizeof(GPUCanvasImageInfo);
        info.size = 2048 * info.stride;
        info.label = "CanvasImageEntries";
        m_infoBuffer = dev->createBuffer(info);
		m_infoBufferSRV = m_infoBuffer->createStructuredView();
    }
}

DynamicAtlas::~DynamicAtlas()
{}

void DynamicAtlas::rebuild()
{
}

bool DynamicAtlas::allocateEntryIndex(uint32_t& outIndex)
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
		m_placements.emplaceBack();
		return true;
	}

	return false;
}

ImageEntry DynamicAtlas::registerImage(const image::Image* ptr, bool supportWrapping /*= false*/, int additionalPixelBorder/*= 0*/)
{
	DEBUG_CHECK_RETURN_EX_V(ptr, "Invalid image", canvas::ImageEntry());

	canvas::ImageEntry ret;

	uint32_t entryIndex = 0;
	if (allocateEntryIndex(entryIndex))
	{
		// return persistent entry data - this stays even if we fail to allocate texture
		ret.atlasIndex = index();
		ret.entryIndex = entryIndex;
		ret.width = ptr->width();
		ret.height = ptr->height();

		// setup entry source data
		auto& entry = m_entries[entryIndex];
		entry.data = AddRef(ptr);
		entry.additionalPixelBorder = additionalPixelBorder;
		entry.supportWrapping = supportWrapping;

		auto& placement = m_placements[entryIndex];
		placement = canvas::ImageAtlasEntryInfo();

		// convert to 4 channels
		if (entry.data->channels() != 4)
			entry.data = image::ConvertChannels(entry.data->view(), 4, &Color::WHITE);

		// add entry to dirty range so it's sent to GPU
		m_dirtyEntryInfoMin = std::min<uint16_t>(m_dirtyEntryInfoMin, entryIndex);
		m_dirtyEntryInfoMax = std::max<uint16_t>(m_dirtyEntryInfoMax, entryIndex + 1);

		// try to place entry in the textures
		placeImage(entry, placement);
	}

	return ret;
}

void DynamicAtlas::unregisterImage(ImageEntry entry)
{
	DEBUG_CHECK_RETURN_EX(entry.entryIndex < m_entries.size(), "Invalid entry index");

	auto* info = m_entries.typedData() + entry.entryIndex;
	DEBUG_CHECK_RETURN_EX(info->data, "Entry is not allocated");
	DEBUG_CHECK_RETURN_EX(info->data->width() == entry.width, "Entry image size does not match");
	DEBUG_CHECK_RETURN_EX(info->data->height() == entry.height, "Entry image size does not match");

	info->data.reset();
	memzero(info, sizeof(Entry));
}

DynamicAtlas::Page::Page(uint32_t size)
	: image(size, size, 4)
{}

bool DynamicAtlas::placeImage(const Entry& source, canvas::ImageAtlasEntryInfo& outPlacement)
{
	// compute size requirements
	int padding = 2;
	padding += source.supportWrapping ? 2 : 0;
	padding += source.additionalPixelBorder;

	// align tile sizes to 4 pixels
	const int requiredWidth = Align<uint32_t>(source.data->width() + padding, 4);
	const int requiredHeight = Align<uint32_t>(source.data->height() + padding, 4);

	// try to allocate in existing space
	for (auto pageIndex : m_pages.indexRange())
	{
		auto& page = m_pages[pageIndex];

		// place the image
		image::DynamicAtlasEntry placedImage;
		if (page.image.placeImage(source.data->view(), padding, source.supportWrapping, placedImage))
		{
			m_dirty = true;
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


const ImageAtlasEntryInfo* DynamicAtlas::findRenderDataForAtlasEntry(ImageEntryIndex entryIndex) const
{
	if (entryIndex < m_placements.size())
	{
		const auto& entry = m_placements[entryIndex];
		if (entry.pageIndex >= 0)
			return &entry;
	}

	return nullptr;
}

//--

void DynamicAtlas::flush(gpu::CommandWriter& cmd)
{
	if (!m_dirty)
		return;

	if (m_dirtyEntryInfoMax > m_dirtyEntryInfoMin)
	{
        cmd.opTransitionLayout(m_infoBuffer, gpu::ResourceLayout::ShaderResource, gpu::ResourceLayout::CopyDest);

        auto* writePtr = cmd.opUpdateDynamicBufferPtrN<GPUCanvasImageInfo>(m_infoBuffer, m_dirtyEntryInfoMin, m_dirtyEntryInfoMax - m_dirtyEntryInfoMin);

        const auto* readPtr = m_placements.typedData() + m_dirtyEntryInfoMin;
        const auto* readEndPtr = m_placements.typedData() + m_dirtyEntryInfoMax;
        while (readPtr < readEndPtr)
        {
            writePtr->uvMin = readPtr->uvOffset;
            writePtr->uvMax = readPtr->uvMax;
            writePtr->uvScale = (readPtr->uvMax - readPtr->uvOffset);
            writePtr->uvInvScale.x = (writePtr->uvScale.x > 0.0f) ? (1.0f / writePtr->uvScale.x) : 0.0f;
            writePtr->uvInvScale.y = (writePtr->uvScale.y > 0.0f) ? (1.0f / writePtr->uvScale.y) : 0.0f;
            ++writePtr;
            ++readPtr;
        }

        cmd.opTransitionLayout(m_infoBuffer, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::ShaderResource);

		m_dirtyEntryInfoMin = m_maxEntries;
		m_dirtyEntryInfoMax = 0;
	}

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
        
END_BOOMER_NAMESPACE_EX(canvas)
