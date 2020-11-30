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
#include "nullApiGraphicsRenderStates.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//--

			GraphicsRenderStates::GraphicsRenderStates(Thread* owner, const GraphicsRenderStatesSetup& setup)
				: IBaseGraphicsRenderStates(owner, setup)
			{}

			GraphicsRenderStates::~GraphicsRenderStates()
			{}

			//--

		} // nul
    } // gl4
} // rendering
