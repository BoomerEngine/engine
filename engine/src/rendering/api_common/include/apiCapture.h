/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

namespace rendering
{
    namespace api
    {
		//---

		// capture interface (triggerable RenderDoc integration)
		class RENDERING_API_COMMON_API IFrameCapture : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_API_RUNTIME);

		public:
			IFrameCapture();
			virtual ~IFrameCapture();

			//--

			static base::UniquePtr<IFrameCapture> ConditionalStartCapture(command::CommandBuffer* masterCommandBuffer);

			//--
		};

		//---

    } // api
} // rendering