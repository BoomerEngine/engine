/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"

#include "canvasAtlas.h"
#include "canvasService.h"
#include "canvasGlyphCache.h"
#include "base/containers/include/bitUtils.h"

BEGIN_BOOMER_NAMESPACE(base::canvas)

//---

ConfigProperty<uint32_t> cvCanvasGlyphCacheAtlasSize("Rendering.Canvas", "GlyphCacheAtlasSize", 2048);
ConfigProperty<uint32_t> cvCanvasGlyphCacheAtlasPageCount("Rendering.Canvas", "GlyphCacheAtlasPageCount", 1);

//---
		
RTTI_BEGIN_TYPE_CLASS(CanvasService);
RTTI_END_TYPE();

//---

CanvasService::CanvasService()
{}

const ImageAtlasEntryInfo* CanvasService::findRenderDataForAtlasEntry(const ImageEntry& entry) const
{
	if (entry.atlasIndex > 0 && entry.atlasIndex < MAX_ATLASES)
		if (const auto* cache = m_atlasRegistry[entry.atlasIndex])
			return cache->findRenderDataForAtlasEntry(entry.entryIndex);

	return nullptr;

}

const ImageAtlasEntryInfo* CanvasService::findRenderDataForGlyph(const font::Glyph* glyph) const
{
	return m_glyphCache->findRenderDataForGlyph(glyph);
}

//--

app::ServiceInitializationResult CanvasService::onInitializeService(const app::CommandLine& cmdLine)
{
	memzero(m_atlasRegistry, sizeof(m_atlasRegistry));

	const auto glyphAtlasSize = NextPow2(std::clamp<uint32_t>(cvCanvasGlyphCacheAtlasSize.get(), 256, 4096));
	const auto glyphAtlasPages = std::clamp<uint32_t>(cvCanvasGlyphCacheAtlasPageCount.get(), 1, 256);
	m_glyphCache = new GlyphCache(glyphAtlasSize, glyphAtlasPages);

	return app::ServiceInitializationResult::Finished;
}

void CanvasService::onShutdownService()
{
	delete m_glyphCache;
	m_glyphCache = nullptr;
}

void CanvasService::onSyncUpdate()
{
	// nothing
}

//--

bool CanvasService::registerAtlas(IAtlas* atlas, ImageAtlasIndex& outIndex)
{
	DEBUG_CHECK_RETURN_EX_V(atlas != nullptr, "Invalid atlas to register", false);
	DEBUG_CHECK_RETURN_EX_V(atlas->index() != 0, "Atlas already registered", false);

	for (uint32_t i = 1; i < MAX_ATLASES; ++i)
	{
		if (!m_atlasRegistry[i])
		{
			m_atlasRegistry[i] = atlas;
			outIndex = i;
			return true;
		}
	}

	TRACE_ERROR("To many image atlases registered");
	return false;
}

void CanvasService::unregisterAtlas(IAtlas* atlas, ImageAtlasIndex index)
{
	DEBUG_CHECK_RETURN_EX(atlas != nullptr, "Invalid atlas to unregister");
	DEBUG_CHECK_RETURN_EX(atlas->index() == index, "Invalid atlas to unregister");
	DEBUG_CHECK_RETURN_EX(index > 0 && index < MAX_ATLASES, "Invalid atlas index");

	DEBUG_CHECK_RETURN_EX(m_atlasRegistry[index] == atlas, "Unexpected atlas");
	m_atlasRegistry[index] = nullptr;
}
		
//--

ICanvasAtlasSync::~ICanvasAtlasSync()
{}

void CanvasService::syncAssetChanges(rendering::GPUCommandWriter& cmd, uint64_t atlasMask, uint64_t glpyhMask, ICanvasAtlasSync& sync)
{
	if (glpyhMask)
		m_glyphCache->flush(cmd, glpyhMask, sync);

	while (atlasMask)
	{
		uint32_t atlasIndex = __builtin_ctzll(atlasMask);
		if (auto* atlas = m_atlasRegistry[atlasIndex])
			atlas->flush(cmd, sync);

		atlasMask ^= atlasMask & -atlasMask;
	}
}

//--

END_BOOMER_NAMESPACE(base::canvas)
