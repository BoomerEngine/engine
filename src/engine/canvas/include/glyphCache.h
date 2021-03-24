/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "core/image/include/imageAtlas.h"
#include "engine/font/include/fontGlyph.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// image cache for font glyphs
class ENGINE_CANVAS_API CanvasGlyphCache : public NoCopy
{
	RTTI_DECLARE_POOL(POOL_CANVAS);

public:
	CanvasGlyphCache(uint32_t size, uint32_t maxPages);
	virtual ~CanvasGlyphCache();

    //--

	/// shader readable view of the atlas's texture
    INLINE const gpu::ImageSampledViewPtr& imageSRV() const { return m_imageSRV; }

	//--

	// find placement of given image in the atlas
	const CanvasImageEntryInfo* findRenderDataForGlyph(const font::Glyph* glyph);

	//--

	// flush changes to renderer
	void flush(gpu::CommandWriter& cmd, uint64_t pageMask);

	//--

private:
	bool m_dirty = false;

	float m_invAtlasSize = 0.0f;
	float m_texelOffset = 0.0f;

	//--

	struct Page
	{
		ImageAtlas image;
		Rect dirtyRect;

		Page(uint32_t size);
	};

	struct Entry
	{
		const font::Glyph* glyph = nullptr;
		CanvasImageEntryInfo placement;
	};

	Array<Page> m_pages;
	HashMap<font::GlyphID, Entry> m_entriesMap;

    //--

    gpu::ImageObjectPtr m_image;
    gpu::ImageSampledViewPtr m_imageSRV;

	//--
};

//--

END_BOOMER_NAMESPACE()


