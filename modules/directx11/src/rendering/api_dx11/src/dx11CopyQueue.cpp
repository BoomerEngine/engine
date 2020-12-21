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

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

	        //--

			CopyQueue::CopyQueue(Thread* owner, ObjectRegistry* objects)
				: IBaseCopyQueue(owner, objects)
			{
			}

			CopyQueue::~CopyQueue()
			{
			}

			bool CopyQueue::schedule(IBaseCopiableObject* ptr, const ISourceDataProvider* sourceData)
			{
				return true;
			}

			void CopyQueue::update(uint64_t frameIndex)
			{

			}

			//--
	
		} // dx11
    } // api
} // rendering
