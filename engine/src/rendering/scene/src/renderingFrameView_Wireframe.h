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

		/// command buffers to write to when recording wireframe view
		struct FrameViewWireframeRecorder : public FrameViewRecorder
		{
			command::CommandWriter viewBegin; // run at the start of the view rendering
			command::CommandWriter viewEnd; // run at the end of the view rendering

			command::CommandWriter depthPrePass; // depth pre pass, mostly used for 
			command::CommandWriter mainSolid; // solid (non transparent) objects - the main part of wireframe stuff
			command::CommandWriter mainTransparent; // transparent objects, not masked (pure solid)
			command::CommandWriter selectionOutline; // objects to render selection outline from
			command::CommandWriter sceneOverlay; // objects to render as scene overlay
			command::CommandWriter screenOverlay; // objects to render as screen overlay (at the screen resolution and after final composition)

			FrameViewWireframeRecorder();
		};

		//--

		/// helper recorder class
		class RENDERING_SCENE_API FrameViewWireframe : public base::NoCopy
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
			~FrameViewWireframe();

			void render(command::CommandWriter& cmd);

			//--

			INLINE const Camera& visibilityCamera() const { return m_camera; }

			//--

		private:
			const FrameRenderer& m_frame;
			const Setup& m_setup;

			base::Rect m_viewport;

			Camera m_camera;

			//--

			void initializeCommandStreams(command::CommandWriter& cmd, FrameViewWireframeRecorder& rec);

			void bindCamera(command::CommandWriter& cmd);
		};
        
        //--

    } // scene
} // rendering

