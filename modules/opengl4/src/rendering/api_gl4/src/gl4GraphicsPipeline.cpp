/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4ObjectCache.h"
#include "gl4GraphicsPipeline.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			GraphicsPipeline::GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsPassLayout* passLayout, const GraphicsRenderStatesSetup& mergedRenderStates)
				: IBaseGraphicsPipeline(owner, shaders, passLayout, mergedRenderStates)
			{
				m_staticRenderStates.apply(mergedRenderStates, m_staticRenderStateMask);
			}

			GraphicsPipeline::~GraphicsPipeline()
			{}

			bool GraphicsPipeline::apply()
			{
				return false;
			}

			//--

		} // gl4
    } // gl4
} // rendering
