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

/// command buffers to write to when recording main view
struct FrameViewMainRecorder : public FrameViewRecorder
{
	GPUCommandWriter viewBegin; // run at the start of the view rendering
	GPUCommandWriter viewEnd; // run at the end of the view rendering

	GPUCommandWriter depthPrePassStatic; // immovable static objects, depth buffer for next frame occlusion culling is captured from this
	GPUCommandWriter depthPrePassOther; // all other (movable) objects that should be part of depth pre-pass
	GPUCommandWriter forwardSolid; // solid (non transparent) objects, not masked (pure solid)
	GPUCommandWriter forwardMasked; // solid (non transparent) objects but with pixel discard or depth modification
	GPUCommandWriter forwardTransparent; // transparent objects
	GPUCommandWriter selectionOutline; // objects to render selection outline from
	GPUCommandWriter sceneOverlay; // objects to render as scene overlay
	GPUCommandWriter screenOverlay; // objects to render as screen overlay (at the screen resolution and after final composition)

	FrameViewMainRecorder();
};

//--

/// frame view command buffers
class RENDERING_SCENE_API FrameViewMain : public FrameViewSingleCamera
{
public:
	struct Setup
	{
		Camera camera;

		const RenderTargetView* colorTarget = nullptr; // NOTE: can be directly a back buffer!
		const RenderTargetView* depthTarget = nullptr;
		base::Rect viewport; // NOTE: does not have to start at 0,0 !!!
		bool flippedY = false;
	};

	//--

	FrameViewMain(const FrameRenderer& frame, const Setup& setup);
	~FrameViewMain();

	void render(GPUCommandWriter& cmd);

	//--

	INLINE const CascadeData& cascades() const { return m_cascades; }

	//--

private:
	const Setup& m_setup;

	CascadeData m_cascades;

	//--

	void initializeCommandStreams(GPUCommandWriter& cmd, FrameViewMainRecorder& rec);

	void bindLighting(GPUCommandWriter& cmd);
};

//--

END_BOOMER_NAMESPACE(rendering::scene)
