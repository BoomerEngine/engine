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

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			base::ConfigProperty<uint32_t> cvCopyQueueStagingAreaSizeMB("Rendering.GL4", "CopyStagingAreaSizeMB", 256);
			base::ConfigProperty<uint32_t> cvCopyQueueStagingAreaPageSize("Rendering.GL4", "CopyQueueStagingAreaPageSize", 4096);

	        //--

			CopyQueue::CopyQueue(Thread* owner, ObjectRegistry* objects)
				: IBaseCopyQueueWithStaging(owner, objects)
				, m_owner(owner)
			{
				// allocate buffer
				m_blockPoolPageSize = base::NextPow2(std::min<uint32_t>(256, cvCopyQueueStagingAreaPageSize.get()));
				m_blockPoolTotalSize = base::Align<uint64_t>((uint64_t)cvCopyQueueStagingAreaSizeMB.get() << 20, m_blockPoolPageSize);
				m_blockPool.setup(m_blockPoolTotalSize, m_blockPoolTotalSize / m_blockPoolPageSize, 1024);
				TRACE_INFO("Allocating resource initialization staging pool, size {}, alignment {}", MemSize(m_blockPoolTotalSize), MemSize(m_blockPoolPageSize));

				// create buffer
				GL_PROTECT(glCreateBuffers(1, &m_glBuffer));

				// label the object
				const base::StringView label = "CopyStagingPool";
				GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, label.length(), label.data()));

				// setup data with buffer
				GL_PROTECT(glNamedBufferStorage(m_glBuffer, m_blockPoolTotalSize, nullptr, GL_CLIENT_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT));

				// persistently map the buffer
				GL_PROTECT(m_stagingAreaPtr = (uint8_t*)glMapNamedBufferRange(m_glBuffer, 0, m_blockPoolTotalSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
				ASSERT_EX(m_stagingAreaPtr != nullptr, "Buffer mapping failed");
			}

			CopyQueue::~CopyQueue()
			{
				stop();

				if (m_stagingAreaPtr)
				{
					GL_PROTECT(glUnmapNamedBuffer(m_glBuffer));
					m_stagingAreaPtr = nullptr;
				}

				if (m_glBuffer)
				{
					GL_PROTECT(glDeleteBuffers(1, &m_glBuffer));
					m_glBuffer = 0;
				}
			}

			IBaseCopyQueueStagingArea* CopyQueue::tryAllocateStagingForJob(const Job& job)
			{
				// compute requirements
				uint32_t memorySizeNeeded = 0;
				uint32_t memoryAlignment = 256;
				for (const auto& atom : job.stagingAtoms)
				{
					auto internalOffset = base::Align<uint32_t>(memorySizeNeeded, atom.alignment);
					memoryAlignment = std::max<uint32_t>(atom.alignment, memoryAlignment);
					memorySizeNeeded = internalOffset + atom.size;
				}

				// try to allocate block
				MemoryBlock block;
				{
					auto lock = CreateLock(m_blockPoolLock);
					if (base::BlockAllocationResult::OK != m_blockPool.allocateBlock(memorySizeNeeded, memoryAlignment, block))
						return nullptr;
				}

				// yay
				TRACE_INFO("Allocated {} in staging buffer ({} total in {} blocks)", MemSize(memorySizeNeeded), 
					MemSize(m_blockPool.numAllocatedBytes()), m_blockPool.numAllocatedBlocks())

				// create staging area wrapper
				auto* ret = new CopyQueueStagingArea;
				ret->glBuffer = m_glBuffer;
				ret->baseOffset = base::BlockPool::GetBlockOffset(block);
				ret->baseDataPtr = m_stagingAreaPtr + ret->baseOffset;
				ret->size = memorySizeNeeded;
				ret->block = block;

				// remember pointers to data
				uint8_t* writePtr = ret->baseDataPtr;
				ret->writeAtoms.reserve(job.stagingAtoms.size());
				for (const auto& atom : job.stagingAtoms)
				{
					auto& writeAtom = ret->writeAtoms.emplaceBack();
					writeAtom.mip = atom.mip;
					writeAtom.slice = atom.slice;
					writeAtom.targetDataSize = atom.size;
					writeAtom.targetDataPtr = base::AlignPtr(writePtr, atom.alignment);
					writeAtom.internalOffset = (uint32_t)(writeAtom.targetDataPtr - m_stagingAreaPtr);
					writePtr = writeAtom.targetDataPtr + atom.size;
				}

				return ret;
			}

			void CopyQueue::releaseStagingArea(IBaseCopyQueueStagingArea* baseArea)
			{
				auto* area = static_cast<CopyQueueStagingArea*>(baseArea);

				auto block = area->block;

				m_owner->registerCompletionCallback(DeviceCompletionType::GPUFrameFinished,
					IDeviceCompletionCallback::CreateFunctionCallback([this, block]()
						{
							auto lock = CreateLock(m_blockPoolLock);
							m_blockPool.freeBlock(block);
						}));

				delete area;
			}

			void CopyQueue::flushStagingArea(IBaseCopyQueueStagingArea* baseArea)
			{
				PC_SCOPE_LVL1(StagingFlush);

				auto* area = static_cast<CopyQueueStagingArea*>(baseArea);
				GL_PROTECT(glFlushMappedNamedBufferRange(area->glBuffer, area->baseOffset, area->size));
			}

			void CopyQueue::copyJobIntoStagingArea(const Job& job)
			{
				PC_SCOPE_LVL1(StagingCopy);

				auto* area = static_cast<CopyQueueStagingArea*>(job.stagingArea);
				job.sourceData->writeSourceData(area->writeAtoms);
			}

			//--
	
		} // gl4
    } // api
} // rendering
