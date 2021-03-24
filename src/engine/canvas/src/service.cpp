/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"

#include "canvas.h"
#include "atlas.h"
#include "service.h"
#include "renderer.h"
#include "glyphCache.h"

#include "core/containers/include/bitUtils.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/shaderService.h"

BEGIN_BOOMER_NAMESPACE()

//---

ConfigProperty<uint32_t> cvCanvasGlyphCacheAtlasSize("Rendering.Canvas", "GlyphCacheAtlasSize", 2048);
ConfigProperty<uint32_t> cvCanvasGlyphCacheAtlasPageCount("Rendering.Canvas", "GlyphCacheAtlasPageCount", 1);

//---
		
RTTI_BEGIN_TYPE_CLASS(CanvasService);
	RTTI_METADATA(DependsOnServiceMetadata).dependsOn<gpu::DeviceService>();
	RTTI_METADATA(DependsOnServiceMetadata).dependsOn<gpu::ShaderService>();	
RTTI_END_TYPE();

//---

CanvasService::CanvasService()
{}

const CanvasImageEntryInfo* CanvasService::findRenderDataForAtlasEntry(const CanvasImageEntry& entry) const
{
	if (entry.atlasIndex > 0 && entry.atlasIndex < MAX_ATLASES)
		if (const auto* cache = m_atlasRegistry[entry.atlasIndex])
			return cache->findRenderDataForAtlasEntry(entry.entryIndex);

	return nullptr;

}

const CanvasImageEntryInfo* CanvasService::findRenderDataForGlyph(const font::Glyph* glyph) const
{
	return m_glyphCache->findRenderDataForGlyph(glyph);
}

//--

bool CanvasService::onInitializeService(const CommandLine& cmdLine)
{
	memzero(m_atlasRegistry, sizeof(m_atlasRegistry));

	const auto glyphAtlasSize = NextPow2(std::clamp<uint32_t>(cvCanvasGlyphCacheAtlasSize.get(), 256, 4096));
	const auto glyphAtlasPages = std::clamp<uint32_t>(cvCanvasGlyphCacheAtlasPageCount.get(), 1, 256);
	m_glyphCache = new CanvasGlyphCache(glyphAtlasSize, glyphAtlasPages);

	m_renderer = new CanvasRenderer();

	return true;
}

void CanvasService::onShutdownService()
{
	delete m_renderer;
	m_renderer = nullptr;

	delete m_glyphCache;
	m_glyphCache = nullptr;
}

void CanvasService::onSyncUpdate()
{
	// nothing
}

//--

bool CanvasService::registerAtlas(ICanvasAtlas* atlas, CanvasAtlasIndex& outIndex)
{
	DEBUG_CHECK_RETURN_EX_V(atlas != nullptr, "Invalid atlas to register", false);
	DEBUG_CHECK_RETURN_EX_V(atlas->index() == 0, "Atlas already registered", false);

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

void CanvasService::unregisterAtlas(ICanvasAtlas* atlas, CanvasAtlasIndex index)
{
	DEBUG_CHECK_RETURN_EX(atlas != nullptr, "Invalid atlas to unregister");
	DEBUG_CHECK_RETURN_EX(atlas->index() == index, "Invalid atlas to unregister");
	DEBUG_CHECK_RETURN_EX(index > 0 && index < MAX_ATLASES, "Invalid atlas index");

	DEBUG_CHECK_RETURN_EX(m_atlasRegistry[index] == atlas, "Unexpected atlas");
	m_atlasRegistry[index] = nullptr;
}
		
//--

void CanvasService::sync(gpu::CommandWriter& cmd, uint64_t atlasMask, uint64_t glpyhMask)
{
    cmd.opBeginBlock("SyncCanvasAtlas");

    if (m_glyphCache)
        m_glyphCache->flush(cmd, glpyhMask);

    while (atlasMask)
    {
        uint32_t atlasIndex = __builtin_ctzll(atlasMask);
        if (auto* atlas = m_atlasRegistry[atlasIndex])
            atlas->flush(cmd);

        atlasMask ^= atlasMask & -atlasMask;
    }

    cmd.opEndBlock();
}

void CanvasService::render(gpu::CommandWriter& cmd, const Canvas& c)
{
	sync(cmd, c.usedAtlasMask(), c.usedGlyphPagesMask());

	CanvasRenderer::RenderInfo info;
	info.glyphs.imageSRV = m_glyphCache->imageSRV();
	info.numValidAtlases = MAX_ATLASES;

	for (uint32_t i = 0; i < MAX_ATLASES; ++i)
	{
		if (auto* atlas = m_atlasRegistry[i])
		{
			info.atlases[i].imageSRV = atlas->imageSRV();
			info.atlases[i].bufferSRV = atlas->imageEntriesSRV();
		}
	}

	m_renderer->render(cmd, info, c);
}

//--

END_BOOMER_NAMESPACE()
