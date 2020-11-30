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
#include "gl4GraphicsRenderStates.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			GraphicsRenderStates::GraphicsRenderStates(Thread* owner, const GraphicsRenderStatesSetup& setup)
				: IBaseGraphicsRenderStates(owner, setup)
			{}

			GraphicsRenderStates::~GraphicsRenderStates()
			{}

			//--

		} // gl4
    } // gl4
} // rendering
