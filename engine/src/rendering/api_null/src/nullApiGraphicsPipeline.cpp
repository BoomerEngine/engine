/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiThread.h"
#include "nullApiObjectCache.h"
#include "nullApiGraphicsPipeline.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//--

			GraphicsPipeline::GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsPassLayout* passLayout, const GraphicsRenderStatesSetup& mergedRenderStates)
				: IBaseGraphicsPipeline(owner, shaders, passLayout, mergedRenderStates)
			{
			}

			GraphicsPipeline::~GraphicsPipeline()
			{}

			//--

		} // nul
    } // gl4
} // rendering
