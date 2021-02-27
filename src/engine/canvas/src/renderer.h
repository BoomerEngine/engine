/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#pragma once

#include "batchRenderer.h"

BEGIN_BOOMER_NAMESPACE_EX(canvas)

//---

/// shared storage of canvas related data, mainly image atlases
class CanvasRenderer : public NoCopy
{
public:
	CanvasRenderer();
    ~CanvasRenderer();

	//--

	struct AtlasInfo
	{
		const gpu::ImageSampledView* imageSRV = nullptr;
		const gpu::BufferStructuredView* bufferSRV = nullptr;
	};

	struct RenderInfo
	{
		static const uint32_t MAX_ATLASES = 64;

		AtlasInfo glyphs;
		AtlasInfo atlases[MAX_ATLASES];
		uint32_t numValidAtlases = 0;
	};

	// render canvas to a given command buffer
	// NOTE: we are expected to be inside a pass
	void render(gpu::CommandWriter& cmd, const RenderInfo& resources, const Canvas& canvas) const;

	//--

private:
	bool m_allowImages = false;

	//--

	gpu::BufferObjectPtr m_sharedVertexBuffer;
	gpu::BufferObjectPtr m_sharedAttributesBuffer;
	gpu::BufferStructuredViewPtr m_sharedAttributesBufferSRV;

	gpu::BufferObjectPtr m_emptyAtlasEntryBuffer;
	gpu::BufferStructuredViewPtr m_emptyAtlasEntryBufferSRV;

	uint32_t m_maxBatchVetices = 0;
	uint32_t m_maxAttributes = 0;

	//--

	gpu::IDevice* m_device = nullptr;

	CanvasRenderStates* m_renderStates = nullptr;

    Array<ICanvasBatchRenderer*> m_batchRenderers;

	//--

	void createRenderStates();
	void createBatchRenderers();
};

//--

END_BOOMER_NAMESPACE_EX(canvas)
