/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "core/image/include/imageView.h"

BEGIN_BOOMER_NAMESPACE()

//--


class CanvasRenderer;
struct CanvasRenderStates;

//--

/// canvas data/rendering service
class ENGINE_CANVAS_API CanvasService : public IService
{
	RTTI_DECLARE_VIRTUAL_CLASS(CanvasService, IService);
	RTTI_DECLARE_POOL(POOL_CANVAS);

public:
	CanvasService();

	//--

	// find placement of given image in the atlas
	// NOTE: atlas must be registered first
	const CanvasImageEntryInfo* findRenderDataForAtlasEntry(const CanvasImageEntry& entry) const;

	// find placement of given font glyph
	const CanvasImageEntryInfo* findRenderDataForGlyph(const FontGlyph* glyph) const;

	//--

	// render canvas to command buffer, flushed atlases
	void render(gpu::CommandWriter& cmd, const Canvas& c);

	//--

private:
	static const uint32_t MAX_ATLASES = 64;

	virtual bool onInitializeService(const CommandLine& cmdLine) override final;
	virtual void onShutdownService() override final;
	virtual void onSyncUpdate() override final;

	ICanvasAtlas* m_atlasRegistry[MAX_ATLASES];

	CanvasGlyphCache* m_glyphCache = nullptr;

	CanvasRenderer* m_renderer = nullptr;

    void sync(gpu::CommandWriter& cmd, uint64_t atlasMask, uint64_t glpyhMask);

	bool registerAtlas(ICanvasAtlas* atlas, CanvasAtlasIndex& outIndex);
	void unregisterAtlas(ICanvasAtlas* atlas, CanvasAtlasIndex index);

	friend class ICanvasAtlas;
};

//--

END_BOOMER_NAMESPACE()
