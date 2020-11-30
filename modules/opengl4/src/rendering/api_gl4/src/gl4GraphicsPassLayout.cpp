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
#include "gl4GraphicsPassLayout.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			GraphicsPassLayout::GraphicsPassLayout(Thread* owner, const rendering::GraphicsPassLayoutSetup& setup)
				: IBaseGraphicsPassLayout(owner, setup)
			{}

			GraphicsPassLayout::~GraphicsPassLayout()
			{}

			//--

		} // gl4
    } // gl4
} // rendering
