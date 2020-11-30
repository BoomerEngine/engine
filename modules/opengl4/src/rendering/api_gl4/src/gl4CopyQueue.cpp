/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4CopyQueue.h"
#include "gl4CopyPool.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
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
	
		} // gl4
    } // api
} // rendering
