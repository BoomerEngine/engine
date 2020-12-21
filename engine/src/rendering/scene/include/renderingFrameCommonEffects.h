/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame #]
***/

#pragma once

namespace rendering
{
    namespace scene
    {
		///---

		/// shared functions, mostly FX
		class RENDERING_SCENE_API FrameCommonEffects : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

		public:
			FrameCommonEffects(IDevice* api); // initialized to the max resolution of the device
			~FrameCommonEffects();

			//--

			

			void finalCompose(command::CommandWriter& cmd, const Setup& setup);

			//--

		private:
			struct TargetFormat
			{
				
			}

			GraphicsPipelineObjectPtr selectComposition(ImageFormat format);
		};

        ///---

    } // scene
} // rendering