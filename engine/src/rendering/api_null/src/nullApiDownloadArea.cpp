/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiThread.h"
#include "nullApiDownloadArea.h"

namespace rendering
{
    namespace api
    {
		namespace nul
		{

			//--

			DownloadArea::DownloadArea(Thread* owner, uint32_t size)
				: IBaseDownloadArea(owner, size)
			{
				m_memoryBlock = base::mem::AllocateBlock(POOL_API_RUNTIME, size, 16);
			}

			DownloadArea::~DownloadArea()
			{
				base::mem::FreeBlock(m_memoryBlock);
				m_memoryBlock = nullptr;
			}

			const void* DownloadArea::retrieveDataPointer_ClientApi()
			{
				return m_memoryBlock;
			}

			//--

		} // nul
    } // gl4
} // rendering
