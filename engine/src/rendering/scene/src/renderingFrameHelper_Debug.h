/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#include "build.h"

namespace rendering
{
    namespace scene
    {
		//--

		class RENDERING_SCENE_API FrameHelperDebug : public base::NoCopy
		{
		public:
			FrameHelperDebug(IDevice* api); // initialized to the max resolution of the device
			~FrameHelperDebug();

			struct Setup
			{
				const Camera* camera = nullptr;

				command::CommandWriter* solid = nullptr;
				command::CommandWriter* transparent = nullptr;
				command::CommandWriter* overlay = nullptr;
			};

            void render(FrameViewRecorder& rec, const FrameParams_DebugGeometry& geom, const Setup& setup) const;

		private:
			IDevice* m_device = nullptr;

			ShaderObjectPtr m_drawShader;

            GraphicsRenderStatesObjectPtr m_renderStatesSolidTriangles;
            GraphicsRenderStatesObjectPtr m_renderStatesSolidLines;
            GraphicsRenderStatesObjectPtr m_renderStatesTransparentTriangles;
            GraphicsRenderStatesObjectPtr m_renderStatesTransparentLines;
            GraphicsRenderStatesObjectPtr m_renderStatesOverlayTriangles;
            GraphicsRenderStatesObjectPtr m_renderStatesOverlayLines;

			GraphicsPipelineObjectPtr m_drawSolid;
			GraphicsPipelineObjectPtr m_drawLines;

			//--

			BufferObjectPtr m_vertexBuffer;
			BufferObjectPtr m_indexBuffer;

			uint32_t m_maxVertexDataSize = 0;
			uint32_t m_maxIndexDataSize = 0;

			void ensureBufferSize(const FrameParams_DebugGeometry& geom);

			void renderInternal(command::CommandWriter& cmd, const DebugGeometry& geom) const;

			//--
		};
        //---

    } // scene
} // rendering

