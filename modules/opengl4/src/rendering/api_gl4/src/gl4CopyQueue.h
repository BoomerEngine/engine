/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiCopyQueue.h"
#include "base/containers/include/blockPool.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//---

			// staging area
			class CopyQueueStagingArea : public IBaseCopyQueueStagingArea
			{
			public:
				INLINE CopyQueueStagingArea() {};
				INLINE ~CopyQueueStagingArea() {};
				
				GLuint glBuffer = 0;

				uint32_t baseOffset = 0;
				uint8_t* baseDataPtr = nullptr;
				uint32_t size = 0;

				base::InplaceArray<ISourceDataProvider::WriteAtom, 12> writeAtoms;

				MemoryBlock block = nullptr;
			};

			//---

			// simple copy queue based on copying data to DMA accessible memory first and than doing copy with GPU
			class CopyQueue : public IBaseCopyQueueWithStaging
			{
			public:
				CopyQueue(Thread* owner, ObjectRegistry* objects);
				virtual ~CopyQueue();

			private:
				Thread* m_owner = nullptr;

				GLuint m_glBuffer = 0;

				uint8_t* m_stagingAreaPtr = nullptr;

				base::SpinLock m_blockPoolLock;
				base::BlockPool m_blockPool;

				uint32_t m_blockPoolTotalSize = 0;
				uint32_t m_blockPoolPageSize = 0;

				//--

				virtual IBaseCopyQueueStagingArea* tryAllocateStagingForJob(const Job& job) override;
				virtual void flushStagingArea(IBaseCopyQueueStagingArea* area) override;
				virtual void releaseStagingArea(IBaseCopyQueueStagingArea* area) override;
				virtual void copyJobIntoStagingArea(const Job& job) override;
			};

			//---

		} // gl4
    } // api
} // rendering