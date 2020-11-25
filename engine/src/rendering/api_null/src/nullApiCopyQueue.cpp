/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "nullApiThread.h"
#include "nullApiCopyQueue.h"
#include "nullApiCopyPool.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

	        //--

			CopyQueue::CopyQueue(Thread* owner, CopyPool* pool, ObjectRegistry* objects)
				: IBaseCopyQueue(owner, pool, objects)
			{

			}

			CopyQueue::~CopyQueue()
			{

			}

			//--
	
		} // nul
    } // api
} // rendering
