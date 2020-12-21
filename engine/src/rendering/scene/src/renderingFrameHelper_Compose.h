/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#pragma once

namespace rendering
{
    namespace scene
    {
		///---

		class RENDERING_SCENE_API FrameHelperCompose : public base::NoCopy
		{
		public:
			FrameHelperCompose(IDevice* api); // initialized to the max resolution of the device
			~FrameHelperCompose();

			struct Setup
			{
				// game viewport
				uint32_t gameWidth = 0;
				uint32_t gameHeight = 0;
				ImageSampledView* gameView = nullptr;

				// output presentation area
				base::Rect presentRect;
				const RenderTargetView* presentTarget = nullptr;

				float gamma = 1.0f;
			};

			void finalCompose(command::CommandWriter& cmd, const Setup& setup) const;

		private:
			ShaderObjectPtr m_blitShaders;
            GraphicsPipelineObjectPtr m_blitShadersPSO;

			//--

			IDevice* m_device = nullptr;
		};		
	
        ///---

    } // scene
} // rendering