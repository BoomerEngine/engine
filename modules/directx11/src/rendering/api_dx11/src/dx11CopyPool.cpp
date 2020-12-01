/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "dx11Thread.h"
#include "dx11CopyPool.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
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
	
		} // dx11
    } // api
} // rendering
