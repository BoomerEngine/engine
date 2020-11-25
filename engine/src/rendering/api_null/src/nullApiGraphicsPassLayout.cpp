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
#include "nullApiGraphicsPassLayout.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//--

			GraphicsPassLayout::GraphicsPassLayout(Thread* owner, const rendering::GraphicsPassLayoutSetup& setup)
				: IBaseGraphicsPassLayout(owner, setup)
			{}

			GraphicsPassLayout::~GraphicsPassLayout()
			{}

			//--

		} // nul
    } // gl4
} // rendering
