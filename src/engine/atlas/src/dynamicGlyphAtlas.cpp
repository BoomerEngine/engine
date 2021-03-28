/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "dynamicGlyphAtlas.h"

#include "engine/font/include/fontGlyphBuffer.h"

BEGIN_BOOMER_NAMESPACE()

//--

DynamicGlyphAtlas::DynamicGlyphAtlas()
	: IDynamicAtlas(ImageFormat::R8_UNORM)
{}

DynamicGlyphAtlas::~DynamicGlyphAtlas()
{}

//--

AtlasImageID DynamicGlyphAtlas::mapGlyph(const FontGlyph& glyph)
{
	if (!glyph.id().fontId)
		return 0;

	auto lock = CreateLock(m_glyphToImageMapLock);

	auto id = 0;
	if (m_glyphToImageMap.find(glyph.id(), id))
		return id;

	{
		auto heavyLock = CreateLock(m_lock);

		const auto rect = Rect(0, 0, glyph.bitmap()->width(), glyph.bitmap()->height());
		id = createEntry_NoLock(glyph.bitmap(), rect, false, false);
	}

	m_glyphToImageMap[glyph.id()] = id;
	return id;
}

void DynamicGlyphAtlas::mapGlyphs(const FontGlyphBuffer& glyphs, Array<AtlasImageID>& outImages)
{
	const auto numGlyphs = glyphs.m_glyphs.size();
	outImages.reserve(numGlyphs);

	auto lock = CreateLock(m_glyphToImageMapLock);

	InplaceArray<uint32_t, 1024> notFoundPositions;
	for (auto i=0; i<numGlyphs; ++i)
	{
		const auto& g = glyphs.m_glyphs[i];

		AtlasImageID id = INDEX_NONE;

		if (g.glyph->bitmap())
		{
			const auto glyphId = g.glyph->id();
			if (!m_glyphToImageMap.find(glyphId, id))
				notFoundPositions.pushBack(i);
		}

		outImages.pushBack(id);
	}

	if (!notFoundPositions.empty())
	{
		auto heavyLock = CreateLock(m_lock);

		for (const auto pos : notFoundPositions)
		{
			const auto* glyph = glyphs.m_glyphs[pos].glyph;

			const auto rect = Rect(0, 0, glyph->bitmap()->width(), glyph->bitmap()->height());
			auto id = createEntry_NoLock(glyph->bitmap(), rect, false, false);

			m_glyphToImageMap[glyph->id()] = id;
			outImages[pos] = id;
		}
	}
}

//--

END_BOOMER_NAMESPACE()
