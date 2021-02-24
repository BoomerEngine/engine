/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#include "build.h"

#include "rendering/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

//--

struct DebugGeometryViewRecorder : public FrameViewRecorder
{
    GPUCommandWriter solid;
    GPUCommandWriter transparent;
    GPUCommandWriter overlay;
    GPUCommandWriter screen;

	DebugGeometryViewRecorder(FrameViewRecorder* parent);
};

class RENDERING_SCENE_API FrameHelperDebug : public base::NoCopy
{
public:
	FrameHelperDebug(IDevice* api); // initialized to the max resolution of the device
	~FrameHelperDebug();

    void render(DebugGeometryViewRecorder& rec, const FrameParams_DebugGeometry& geom, const Camera* camera) const;

private:
	IDevice* m_device = nullptr;

	ShaderObjectPtr m_drawShaderSolid;
	ShaderObjectPtr m_drawShaderLines;

	struct Shaders
	{
        GraphicsPipelineObjectPtr drawTriangles;
        GraphicsPipelineObjectPtr drawLines;
	};

	Shaders m_renderStatesSolid;
	Shaders m_renderStatesTransparent;
	Shaders m_renderStatesOverlay;
	Shaders m_renderStatesScreen;
			
	//--

	mutable BufferObjectPtr m_vertexBuffer;
	mutable BufferObjectPtr m_indexBuffer;
	mutable uint32_t m_maxVertexDataSize = 0;
	mutable uint32_t m_maxIndexDataSize = 0;

	void ensureBufferSize(const FrameParams_DebugGeometry& geom) const;

	void renderInternal(GPUCommandWriter& cmd, const Camera* camera, const DebugGeometry& geom, const Shaders& shaders) const;

	//--
};

//---

END_BOOMER_NAMESPACE(rendering::scene)

