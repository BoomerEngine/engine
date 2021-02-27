/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderer.h"
#include "view.h"
#include "viewCascades.h"

#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//--

/// command buffers to write to when recording main view
struct FrameViewMainRecorder : public FrameViewRecorder
{
	gpu::CommandWriter viewBegin; // run at the start of the view rendering
	gpu::CommandWriter viewEnd; // run at the end of the view rendering

	gpu::CommandWriter depthPrePassStatic; // immovable static objects, depth buffer for next frame occlusion culling is captured from this
	gpu::CommandWriter depthPrePassOther; // all other (movable) objects that should be part of depth pre-pass
	gpu::CommandWriter forwardSolid; // solid (non transparent) objects, not masked (pure solid)
	gpu::CommandWriter forwardMasked; // solid (non transparent) objects but with pixel discard or depth modification
	gpu::CommandWriter forwardTransparent; // transparent objects
	gpu::CommandWriter selectionOutline; // objects to render selection outline from
	gpu::CommandWriter sceneOverlay; // objects to render as scene overlay
	gpu::CommandWriter screenOverlay; // objects to render as screen overlay (at the screen resolution and after final composition)

	FrameViewMainRecorder();
};

//--

/// frame view command buffers
class ENGINE_RENDERING_API FrameViewMain : public FrameViewSingleCamera
{
public:
	struct Setup
	{
		Camera camera;

		const gpu::RenderTargetView* colorTarget = nullptr; // NOTE: can be directly a back buffer!
		const gpu::RenderTargetView* depthTarget = nullptr;
		Rect viewport; // NOTE: does not have to start at 0,0 !!!
		bool flippedY = false;
	};

	//--

	FrameViewMain(const FrameRenderer& frame, const Setup& setup);
	~FrameViewMain();

	void render(gpu::CommandWriter& cmd);

	//--

	INLINE const CascadeData& cascades() const { return m_cascades; }

	//--

private:
	const Setup& m_setup;

	CascadeData m_cascades;

	//--

	void initializeCommandStreams(gpu::CommandWriter& cmd, FrameViewMainRecorder& rec);

	void bindLighting(gpu::CommandWriter& cmd);
};

//--

END_BOOMER_NAMESPACE_EX(rendering)
