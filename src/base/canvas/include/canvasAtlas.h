/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "base/image/include/imageDynamicAtlas.h"

BEGIN_BOOMER_NAMESPACE(base::canvas)

//--

/// abstract canvas atlas
class BASE_CANVAS_API IAtlas : public NoCopy
{
public:
	IAtlas();
	virtual ~IAtlas();

	// get internal atlas index
	INLINE ImageAtlasIndex index() const { return m_index; }

	//--

	// find placement of given image in the atlas
	virtual const ImageAtlasEntryInfo* findRenderDataForAtlasEntry(ImageEntryIndex index) const = 0;

	// flush any data changed to rendering interface
	virtual void flush(rendering::GPUCommandWriter& cmd, ICanvasAtlasSync& renderer) = 0;

	//--

private:
	ImageAtlasIndex m_index;
};

//--

/// simple dynamic atlas that can be built from images
class BASE_CANVAS_API DynamicAtlas : public IReferencable, public IAtlas
{
public:
	DynamicAtlas(uint32_t size, uint32_t maxPages);
	virtual ~DynamicAtlas();

	//--

	// rebuild atlas, reclaim unused space
	void rebuild();

	//--

	// place image in the atlas, this might fail if atlas is invalid or we run out of space in it
	ImageEntry registerImage(const image::Image* ptr, bool supportWrapping = false, int additionalPixelBorder = 0);

	// remove image from the atlas
	void unregisterImage(ImageEntry entry);

	//--

	// find placement of given image in the atlas
	const ImageAtlasEntryInfo* findRenderDataForAtlasEntry(ImageEntryIndex index) const;

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
		image::ImagePtr data;

		uint8_t supportWrapping = 0;
		uint8_t additionalPixelBorder = 0;
	};

	struct Page
	{
		image::DynamicAtlas image;
		Rect dirtyRect;

		Page(uint32_t size);
	};

	Array<Page> m_pages;

	Array<uint16_t> m_freeEntryIndices;
	Array<Entry> m_entries;
	Array<ImageAtlasEntryInfo> m_placements;

	//--

	bool allocateEntryIndex(uint32_t& outIndex);
	bool placeImage(const Entry& source, canvas::ImageAtlasEntryInfo& outPlacement);

	virtual void flush(rendering::GPUCommandWriter& cmd, ICanvasAtlasSync& renderer) override final;
};

//--

END_BOOMER_NAMESPACE(base::canvas)
