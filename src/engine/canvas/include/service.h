/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "core/image/include/imageView.h"

BEGIN_BOOMER_NAMESPACE_EX(canvas)

//--


class CanvasRenderer;
struct CanvasRenderStates;

//--

/// canvas data/rendering service
class ENGINE_CANVAS_API CanvasService : public app::ILocalService
{
	RTTI_DECLARE_VIRTUAL_CLASS(CanvasService, app::ILocalService);
	RTTI_DECLARE_POOL(POOL_CANVAS);

public:
	CanvasService();

	//--

	// find placement of given image in the atlas
	// NOTE: atlas must be registered first
	const ImageAtlasEntryInfo* findRenderDataForAtlasEntry(const ImageEntry& entry) const;

	// find placement of given font glyph
	const ImageAtlasEntryInfo* findRenderDataForGlyph(const font::Glyph* glyph) const;

	//--

	// render canvas to command buffer, flushed atlases
	void render(gpu::CommandWriter& cmd, const Canvas& c);

	//--

private:
	static const uint32_t MAX_ATLASES = 64;

	virtual app::ServiceInitializationResult onInitializeService(const app::CommandLine& cmdLine) override final;
	virtual void onShutdownService() override final;
	virtual void onSyncUpdate() override final;

	IAtlas* m_atlasRegistry[MAX_ATLASES];

	GlyphCache* m_glyphCache = nullptr;

	CanvasRenderer* m_renderer = nullptr;

    void sync(gpu::CommandWriter& cmd, uint64_t atlasMask, uint64_t glpyhMask);

	bool registerAtlas(IAtlas* atlas, ImageAtlasIndex& outIndex);
	void unregisterAtlas(IAtlas* atlas, ImageAtlasIndex index);

	friend class IAtlas;
};

//--

END_BOOMER_NAMESPACE_EX(canvas)
