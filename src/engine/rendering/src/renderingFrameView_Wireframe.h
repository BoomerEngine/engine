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

#include "gpu/device/include/renderingCommandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

/// command buffers to write to when recording wireframe view
struct FrameViewWireframeRecorder : public FrameViewRecorder
{
	gpu::CommandWriter viewBegin; // run at the start of the view rendering
	gpu::CommandWriter viewEnd; // run at the end of the view rendering

	gpu::CommandWriter depthPrePass; // depth pre pass, mostly used for
	gpu::CommandWriter mainSolid; // solid (non transparent) objects - the main part of wireframe stuff
	gpu::CommandWriter mainTransparent; // transparent objects, not masked (pure solid)
	gpu::CommandWriter selectionOutline; // objects to render selection outline from
	gpu::CommandWriter sceneOverlay; // objects to render as scene overlay
	gpu::CommandWriter screenOverlay; // objects to render as screen overlay (at the screen resolution and after final composition)

	FrameViewWireframeRecorder();
};

//--

/// helper recorder class
class ENGINE_RENDERING_API FrameViewWireframe : public FrameViewSingleCamera
{
public:
	struct Setup
	{
		Camera camera;

		const gpu::RenderTargetView* colorTarget = nullptr; // NOTE: can be directly a back buffer!
		const gpu::RenderTargetView* depthTarget = nullptr;
		Rect viewport; // NOTE: does not have to start at 0,0 !!!
	};

	//--

	FrameViewWireframe(const FrameRenderer& frame, const Setup& setup);

	void render(gpu::CommandWriter& cmd);

	//--

private:
	const Setup& m_setup;

	void initializeCommandStreams(gpu::CommandWriter& cmd, FrameViewWireframeRecorder& rec);
};

//--

END_BOOMER_NAMESPACE_EX(rendering)
