/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "core/image/include/imageAtlas.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// abstract canvas atlas
class ENGINE_CANVAS_API ICanvasAtlas : public NoCopy
{
public:
	ICanvasAtlas();
	virtual ~ICanvasAtlas();

	//--

	// get internal atlas index
	INLINE CanvasAtlasIndex index() const { return m_index; }

	//--

	// find placement of given image in the atlas
	virtual const CanvasImageEntryInfo* findRenderDataForAtlasEntry(CanvasImageIndex index) const = 0;

	// flush any data changed to rendering interface
	virtual void flush(gpu::CommandWriter& cmd) = 0;

    /// shader readable view of the atlas's texture
    virtual const gpu::ImageSampledViewPtr& imageSRV() const = 0;

    /// shader readable buffer with information about entries in the atlas
    virtual const gpu::BufferStructuredViewPtr& imageEntriesSRV() const = 0;

	//--

private:
	CanvasAtlasIndex m_index = 0;
};

//--

/// simple dynamic atlas that can be built from images
class ENGINE_CANVAS_API CanvasDynamicAtlas : public IReferencable, public ICanvasAtlas
{
public:
	CanvasDynamicAtlas(uint32_t size, uint32_t maxPages);
	virtual ~CanvasDynamicAtlas();

	//--

	/// shader readable view of the atlas's texture
    virtual const gpu::ImageSampledViewPtr& imageSRV() const override { return m_imageSRV; }

    /// shader readable buffer with information about entries in the atlas
	virtual const gpu::BufferStructuredViewPtr& imageEntriesSRV() const override { return m_infoBufferSRV; }

	//--

	// rebuild atlas, reclaim unused space
	void rebuild();

	//--

	// place image in the atlas, this might fail if atlas is invalid or we run out of space in it
	CanvasImageEntry registerImage(const Image* ptr, bool supportWrapping = false, int additionalPixelBorder = 0);

	// remove image from the atlas
	void unregisterImage(CanvasImageEntry entry);

	//--

	// find placement of given image in the atlas
	const CanvasImageEntryInfo* findRenderDataForAtlasEntry(CanvasImageIndex index) const;

private:
	bool m_dirty = false;

	uint16_t m_maxEntries = 0;

	float m_invAtlasWidth = 0.0f;
	float m_invAtlasHeight = 0.0f;

	uint16_t m_dirtyEntryInfoMin = 0;
	uint16_t m_dirtyEntryInfoMax = 0;

	//--

	struct Entry
	{
		ImagePtr data;

		uint8_t supportWrapping = 0;
		uint8_t additionalPixelBorder = 0;
	};

	struct Page
	{
		ImageAtlas image;
		Rect dirtyRect;

		Page(uint32_t size);
	};

	Array<Page> m_pages;

	Array<uint16_t> m_freeEntryIndices;
	Array<Entry> m_entries;
	Array<CanvasImageEntryInfo> m_placements;

	//--

	gpu::ImageObjectPtr m_image;
	gpu::ImageSampledViewPtr m_imageSRV;

	gpu::BufferObjectPtr m_infoBuffer;
	gpu::BufferStructuredViewPtr m_infoBufferSRV;

	//--

	bool allocateEntryIndex(uint32_t& outIndex);
	bool placeImage(const Entry& source, CanvasImageEntryInfo& outPlacement);

	//--

	virtual void flush(gpu::CommandWriter& cmd) override final;
};

//--

END_BOOMER_NAMESPACE()
