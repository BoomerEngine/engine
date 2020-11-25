/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "nullApiThread.h"
#include "nullApiCopyPool.h"

namespace rendering
{
    namespace api
    {
		namespace nul
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
	
		} // nul
    } // api
} // rendering
