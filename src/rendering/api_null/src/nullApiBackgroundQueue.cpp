/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "nullApiBackgroundQueue.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

	        //--

			BackgroundQueue::BackgroundQueue()
			{}

			bool BackgroundQueue::createWorkerThreads(uint32_t requestedCount, uint32_t& outNumCreated)
			{
				outNumCreated = 0;
				return true;
			}

			void BackgroundQueue::stopWorkerThreads()
			{}

			//--
	
		} // nul
    } // api
} // rendering
