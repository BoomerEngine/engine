/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#include "build.h"

#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE()

//--

struct DebugGeometryViewRecorder : public FrameViewRecorder
{
    gpu::CommandWriter solid;
    gpu::CommandWriter transparent;
    gpu::CommandWriter overlay;
    gpu::CommandWriter screen;

	DebugGeometryViewRecorder(FrameViewRecorder* parent);
};

class ENGINE_RENDERING_API FrameHelperDebug : public NoCopy
{
public:
	FrameHelperDebug(gpu::IDevice* api); // initialized to the max resolution of the device
	~FrameHelperDebug();

    void render(DebugGeometryViewRecorder& rec, const DebugGeometryCollector* debugGeometry, const Camera* camera) const;

private:
	gpu::IDevice* m_device = nullptr;

	gpu::GraphicsPipelineObjectPtr m_renderStatesSolid;
	gpu::GraphicsPipelineObjectPtr m_renderStatesTransparent;
	gpu::GraphicsPipelineObjectPtr m_renderStatesOverlay;
	gpu::GraphicsPipelineObjectPtr m_renderStatesSelection;
			
	//--

	mutable gpu::BufferObjectPtr m_vertexBuffer;
	mutable gpu::BufferObjectPtr m_indexBuffer;
	mutable uint32_t m_maxVertexDataSize = 0;
	mutable uint32_t m_maxIndexDataSize = 0;

	void ensureBufferSize(const DebugGeometryCollector* debugGeometry) const;

	void renderInternal(gpu::CommandWriter& cmd, const Camera* camera, const DebugGeometryCollector* debugGeometry, DebugGeometryLayer layer, const gpu::GraphicsPipelineObject* shader) const;

	//--
};

//---

END_BOOMER_NAMESPACE()

