/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
***/

#pragma once

#include "nullApiShaders.h"
#include "nullApiGraphicsRenderStates.h"
#include "nullApiGraphicsPassLayout.h"

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
				GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsPassLayout* passLayout, const GraphicsRenderStates* states);
				virtual ~GraphicsPipeline();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
				INLINE const Shaders* shaders() const { return static_cast<const Shaders*>(IBaseGraphicsPipeline::shaders()); }
				INLINE const GraphicsPassLayout* passLayout() const { return static_cast<const GraphicsPassLayout*>(IBaseGraphicsPipeline::passLayout()); }
				INLINE const GraphicsRenderStates* renderStates() const { return static_cast<const GraphicsRenderStates*>(IBaseGraphicsPipeline::renderStates()); }

				//--				

			private:
			};

			//--

		} // nul
    } // api
} // rendering
