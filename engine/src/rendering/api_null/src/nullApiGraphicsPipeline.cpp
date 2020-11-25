/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
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

			GraphicsPipeline::GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsPassLayout* passLayout, const GraphicsRenderStates* states)
				: IBaseGraphicsPipeline(owner, shaders, passLayout, states)
			{
			}

			GraphicsPipeline::~GraphicsPipeline()
			{}

			//--

		} // nul
    } // gl4
} // rendering
