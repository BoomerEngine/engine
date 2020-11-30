/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4CopyPool.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

	        //--

			CopyPool::CopyPool(uint32_t size, uint32_t pageSize)
				: IBaseStagingPool(size, pageSize)
			{
			}

			CopyPool::~CopyPool()
			{
			}

			//--
	
		} // gl4
    } // api
} // rendering
