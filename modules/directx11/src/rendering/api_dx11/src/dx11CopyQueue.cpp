/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "dx11Thread.h"
#include "dx11CopyQueue.h"
#include "dx11CopyPool.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
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
	
		} // dx11
    } // api
} // rendering
