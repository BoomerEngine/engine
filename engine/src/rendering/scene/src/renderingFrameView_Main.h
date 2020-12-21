/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\view #]
***/

#include "build.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameView_Cascades.h"

#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
    namespace scene
    {

		//--

		/// command buffers to write to when recording main view
		struct FrameViewMainRecorder : public FrameViewRecorder
		{
			command::CommandWriter viewBegin; // run at the start of the view rendering
			command::CommandWriter viewEnd; // run at the end of the view rendering

			command::CommandWriter depthPrePassStatic; // immovable static objects, depth buffer for next frame occlusion culling is captured from this
			command::CommandWriter depthPrePassOther; // all other (movable) objects that should be part of depth pre-pass
			command::CommandWriter forwardSolid; // solid (non transparent) objects, not masked (pure solid)
			command::CommandWriter forwardMasked; // solid (non transparent) objects but with pixel discard or depth modification
			command::CommandWriter forwardTransparent; // transparent objects
			command::CommandWriter selectionOutline; // objects to render selection outline from
			command::CommandWriter sceneOverlay; // objects to render as scene overlay

			FrameViewMainRecorder();
		};

		//--

		/// frame view command buffers
		class RENDERING_SCENE_API FrameViewMain : public base::NoCopy
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

			FrameViewMain(const FrameRenderer& frame, const Setup& setup);
			~FrameViewMain();

			void render(command::CommandWriter& cmd);

			//--

		private:
			const FrameRenderer& m_frame;
			const Setup& m_setup;

			CascadeData m_cascades;

			//--

			void initializeCommandStreams(command::CommandWriter& cmd, FrameViewMainRecorder& rec);

			void bindCamera(command::CommandWriter& cmd);
			
			void renderFragments(Scene* scene, FrameViewMainRecorder& rec);
		};
        
        //--

    } // scene
} // rendering

