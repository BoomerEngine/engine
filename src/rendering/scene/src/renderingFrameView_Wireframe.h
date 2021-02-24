/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameView.h"
#include "renderingFrameView_Cascades.h"

#include "rendering/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

//--

/// command buffers to write to when recording wireframe view
struct FrameViewWireframeRecorder : public FrameViewRecorder
{
	GPUCommandWriter viewBegin; // run at the start of the view rendering
	GPUCommandWriter viewEnd; // run at the end of the view rendering

	GPUCommandWriter depthPrePass; // depth pre pass, mostly used for
	GPUCommandWriter mainSolid; // solid (non transparent) objects - the main part of wireframe stuff
	GPUCommandWriter mainTransparent; // transparent objects, not masked (pure solid)
	GPUCommandWriter selectionOutline; // objects to render selection outline from
	GPUCommandWriter sceneOverlay; // objects to render as scene overlay
	GPUCommandWriter screenOverlay; // objects to render as screen overlay (at the screen resolution and after final composition)

	FrameViewWireframeRecorder();
};

//--

/// helper recorder class
class RENDERING_SCENE_API FrameViewWireframe : public FrameViewSingleCamera
{
public:
	struct Setup
	{
		Camera camera;

		const RenderTargetView* colorTarget = nullptr; // NOTE: can be directly a back buffer!
		const RenderTargetView* depthTarget = nullptr;
		base::Rect viewport; // NOTE: does not have to start at 0,0 !!!
	};

	//--

	FrameViewWireframe(const FrameRenderer& frame, const Setup& setup);

	void render(GPUCommandWriter& cmd);

	//--

private:
	const Setup& m_setup;

	void initializeCommandStreams(GPUCommandWriter& cmd, FrameViewWireframeRecorder& rec);
};

//--

END_BOOMER_NAMESPACE(rendering::scene)