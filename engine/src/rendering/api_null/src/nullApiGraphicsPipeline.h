/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "nullApiShaders.h"

#include "rendering/api_common/include/apiGraphicsPipeline.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{
			///--

			class GraphicsPipeline : public IBaseGraphicsPipeline
			{
			public:
				GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsRenderStatesSetup& mergedRenderStates);
				virtual ~GraphicsPipeline();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
				INLINE const Shaders* shaders() const { return static_cast<const Shaders*>(IBaseGraphicsPipeline::shaders()); }

				//--				

			private:
			};

			//--

		} // nul
    } // api
} // rendering

