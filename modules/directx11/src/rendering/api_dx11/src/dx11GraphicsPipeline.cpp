/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
***/

#include "build.h"
#include "dx11Thread.h"
#include "dx11ObjectCache.h"
#include "dx11GraphicsPipeline.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			//--

			GraphicsPipeline::GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsRenderStatesSetup& mergedRenderStates)
				: IBaseGraphicsPipeline(owner, shaders, mergedRenderStates)
			{
			}

			GraphicsPipeline::~GraphicsPipeline()
			{}

			//--

		} // dx11
    } // gl4
} // rendering
