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

namespace rendering
{
    namespace api
    {
		namespace nul
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
	
		} // nul
    } // api
} // rendering
