/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
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

			GraphicsPipeline::GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsPassLayout* passLayout, const GraphicsRenderStates* states)
				: IBaseGraphicsPipeline(owner, shaders, passLayout, states)
			{
			}

			GraphicsPipeline::~GraphicsPipeline()
			{}

			//--

		} // gl4
    } // gl4
} // rendering
