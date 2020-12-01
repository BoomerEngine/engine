/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "dx11Thread.h"
#include "dx11ObjectCache.h"
#include "dx11GraphicsPassLayout.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			//--

			GraphicsPassLayout::GraphicsPassLayout(Thread* owner, const rendering::GraphicsPassLayoutSetup& setup)
				: IBaseGraphicsPassLayout(owner, setup)
			{}

			GraphicsPassLayout::~GraphicsPassLayout()
			{}

			//--

		} // dx11
    } // gl4
} // rendering
