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

    INLINE const DynamicGlyphAtlasPtr& glyphAtlas() const { return m_glyphAtlas; }

    INLINE const DynamicImageAtlasPtr& imageAtlas() const { return m_imageAtlas; }
	
	//--

	// render canvas to command buffer, flushed atlases
	void render(gpu::CommandWriter& cmd, const Canvas& c);

	//--

private:
	static const uint32_t MAX_ATLASES = 64;

	virtual bool onInitializeService(const CommandLine& cmdLine) override final;
	virtual void onShutdownService() override final;
	virtual void onSyncUpdate() override final;

	CanvasRenderer* m_renderer = nullptr;

    void sync(gpu::CommandWriter& cmd);

    DynamicGlyphAtlasPtr m_glyphAtlas;
    DynamicImageAtlasPtr m_imageAtlas;

	friend class ICanvasAtlas;
};

//--

END_BOOMER_NAMESPACE()
