/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "glObject.h"
#include "glObjectRegistry.h"
#include "glBuffer.h"
#include "glImage.h"
#include "glDevice.h"
#include "glDeviceThreadCopy.h"
#include "glFrame.h"
#include "base/system/include/thread.h"

namespace rendering
{
    namespace gl4
    {
		//--

		DeviceCopyStagingPool::DeviceCopyStagingPool(Device* drv, uint32_t size, uint32_t pageSize)
			: m_blockPoolTotalSize(size)
			, m_blockPoolPageSize(pageSize)
		{
			// create buffer
			GL_PROTECT(glCreateBuffers(1, &m_glBuffer));

			// label the object
			const base::StringView label = "CopyStagingPool";
			GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, label.length(), label.data()));

			// setup data with buffer
			GL_PROTECT(glNamedBufferStorage(m_glBuffer, size, nullptr, GL_CLIENT_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT));

			// initialize the storage, track blocks roughly in page size quanta, don't bother tracking smaller blocks directly (they are recovered though)
			DEBUG_CHECK(base::IsPow2(pageSize));
			DEBUG_CHECK(size % pageSize == 0);
			m_blockPool.setup(size, size / pageSize, pageSize);
			TRACE_INFO("GL: Allocated staging pool {}, {} blocks ({} page size)", MemSize(size), size / pageSize, MemSize(pageSize));

			// persistently map the buffer
			GL_PROTECT(m_stagingAreaPtr = (uint8_t*)glMapNamedBufferRange(m_glBuffer, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
			ASSERT_EX(m_stagingAreaPtr != nullptr, "Buffer mapping failed");
		}

		DeviceCopyStagingPool::~DeviceCopyStagingPool()
		{
			if (m_glBuffer)
			{
				GL_PROTECT(glDeleteBuffers(1, &m_glBuffer));
				m_glBuffer = 0;
			}
		}

		//---

		DeviceCopyStagingArea DeviceCopyStagingPool::allocate(uint32_t size, base::StringView label)
		{
			DEBUG_CHECK_RETURN_V(size <= m_blockPoolTotalSize, DeviceCopyStagingArea());

			MemoryBlock allocatedBlock;

			// allocate needed memory from block pool
			{
				auto lock = CreateLock(m_lock);

				const auto ret = m_blockPool.allocateBlock(size, m_blockPoolPageSize, allocatedBlock);
				if (ret != base::BlockAllocationResult::OK)
				{
					TRACE_SPAM("GL: Allocation of staging block {} failed ({})", MemSize(size), label);
					return DeviceCopyStagingArea();
				}

				const auto offset = base::BlockPool::GetBlockOffset(allocatedBlock);
				TRACE_SPAM("GL: Allocated staging block {} @ {} for '{}', {} blocks allocated ({})", MemSize(size), offset, label, 
					m_blockPool.numAllocatedBlocks(), MemSize(m_blockPool.numAllocatedBytes()));
			}

			// yay, prepare staging area
			DeviceCopyStagingArea ret;
			ret.block = allocatedBlock;
			ret.bufferOffset = base::BlockPool::GetBlockOffset(allocatedBlock);
			ret.bufferSize = size;
			ret.dataPtr = m_stagingAreaPtr + ret.bufferOffset;
			ret.glBuffer = m_glBuffer;
			ret.timestamp = base::NativeTimePoint::Now();

#ifndef BUILD_RELEASE
			ret.label = base::StringBuf(label);
#endif

			return ret;				
		}

		void DeviceCopyStagingPool::free(const DeviceCopyStagingArea& area)
		{
			DEBUG_CHECK_RETURN(!!area);
			DEBUG_CHECK_RETURN(area.glBuffer == m_glBuffer);
			DEBUG_CHECK_RETURN(area.bufferOffset <= m_blockPoolTotalSize);
			DEBUG_CHECK_RETURN(area.bufferOffset + area.bufferSize <= m_blockPoolTotalSize);

			{
				auto lock = CreateLock(m_lock);
				m_blockPool.freeBlock(area.block);

#ifndef BUILD_RELEASE
				base::StringView label = area.label;
#else
				base::StringView label;
#endif

				TRACE_SPAM("GL: Freed staging block {} @ {} for '{}', {} blocks allocated ({})", MemSize(area.bufferSize), area.bufferOffset, label,
					m_blockPool.numAllocatedBlocks(), MemSize(m_blockPool.numAllocatedBytes()));
			}
		}

		ResolvedBufferView DeviceCopyStagingPool::flushWrites(const DeviceCopyStagingArea& area)
		{
			// TODO: merge writes ?
			GL_PROTECT(glFlushMappedNamedBufferRange(m_glBuffer, area.bufferOffset, area.bufferSize));

			ResolvedBufferView ret;
			ret.glBuffer = m_glBuffer;
			ret.offset = area.bufferOffset;
			ret.size = area.bufferSize;
			return ret;
		}

		//--	
		
		DeviceCopyQueue::DeviceCopyQueue(Device* drv, DeviceCopyStagingPool* pool, ObjectRegistry* objects)
			: m_pool(pool)
			, m_objects(objects)
		{
			DEBUG_CHECK(m_pool != nullptr);
			DEBUG_CHECK(drv != nullptr);
			DEBUG_CHECK(m_objects != nullptr);

			static const uint32_t NUM_INITIAL_JOBS = 1024;

			m_pendingJobs.reserve(NUM_INITIAL_JOBS);
			m_processingJobs.reserve(NUM_INITIAL_JOBS);
			m_tempJobList.reserve(NUM_INITIAL_JOBS);
		}

		DeviceCopyQueue::~DeviceCopyQueue()
		{
			stop();
		}

		bool DeviceCopyQueue::scheduleAsync_ClientApi(Object* ptr, const ResourceCopyRange& range, const ISourceDataProvider* sourceData, base::fibers::WaitCounter fence)
		{
			DEBUG_CHECK_RETURN_V(ptr != nullptr, false);
			DEBUG_CHECK_RETURN_V(sourceData != nullptr, false);
			DEBUG_CHECK_RETURN_V(ptr->objectType() == ObjectType::Buffer || ptr->objectType() == ObjectType::Image, false);

			auto context = base::RefPtr<CopyContext>();

			if (ptr->objectType() == ObjectType::Buffer)
			{
				const auto* buffer = static_cast<const Buffer*>(ptr);
				DEBUG_CHECK_RETURN_V(range.buffer.offset <= buffer->dataSize(), false);
				DEBUG_CHECK_RETURN_V(range.buffer.offset + range.buffer.size <= buffer->dataSize(), false);

				auto* job = new Job();
				job->id = ptr->handle();
				job->fence = fence;
				job->sourceData = AddRef(sourceData);
				job->timestamp = base::NativeTimePoint::Now();
				job->stagingAreaSize = range.buffer.size;
				job->range = range;

				{
					auto lock = CreateLock(m_pendingJobLock);
					m_pendingJobs.push(AddRef(job));
					TRACE_SPAM("GL: Created pending async copy buffer job for '{}', size {}", sourceData->debugLabel(), range.buffer.size);
				}
			}
			else if  (ptr->objectType() == ObjectType::Image)
			{
				const auto* image = static_cast<const Image*>(ptr);
				const auto& setup = image->creationSetup();

				DEBUG_CHECK_RETURN_V(range.image.firstMip < setup.numMips, false);
				DEBUG_CHECK_RETURN_V(range.image.firstSlice < setup.numSlices, false);
				DEBUG_CHECK_RETURN_V(range.image.firstMip + range.image.numMips <= setup.numMips, false);
				DEBUG_CHECK_RETURN_V(range.image.firstSlice + range.image.numSlices <= setup.numSlices, false);

				DEBUG_CHECK_RETURN_V(range.image.firstMip < setup.numMips, false);
				const auto mipWidth = image->creationSetup().width >> range.image.firstMip;
				const auto mipHeight = image->creationSetup().height >> range.image.firstMip;
				const auto mipDepth = image->creationSetup().depth >> range.image.firstMip;
				DEBUG_CHECK_RETURN_V(range.image.offsetX < mipWidth, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetY < mipHeight, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetZ < mipDepth, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetX + range.image.sizeX <= mipWidth, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetY + range.image.sizeY <= mipHeight, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetZ + range.image.sizeZ <= mipDepth, false);


				// create a separate job for each mip/slice (in OpenGL we upload data independently any way) and we don't have to use contiguous staging area
				{
					auto lock = CreateLock(m_pendingJobLock);
					auto timestamp = base::NativeTimePoint::Now();

					for (uint32_t slice=0; slice<range.image.numSlices; ++slice)
					{
						for (uint32_t mip=0; mip<range.image.numMips; ++mip)
						{
							auto* job = new Job();
							job->id = ptr->handle();
							job->fence = fence;
							job->range.image.format = image->creationSetup().format;
							job->range.image.numMips = 1;
							job->range.image.numSlices = 1;
							job->range.image.firstMip = mip + range.image.firstMip;
							job->range.image.firstSlice = slice + range.image.firstSlice;
							job->range.image.offsetX = range.image.offsetX >> mip;
							job->range.image.offsetY = range.image.offsetY >> mip;
							job->range.image.offsetZ = range.image.offsetZ >> mip;
							job->range.image.sizeX = std::max<uint32_t>(1, image->creationSetup().width >> mip);
							job->range.image.sizeY = std::max<uint32_t>(1, image->creationSetup().height >> mip);
							job->range.image.sizeZ = std::max<uint32_t>(1, image->creationSetup().depth >> mip);
							job->timestamp = timestamp;
							job->stagingAreaSize = setup.calcMipDataSize(mip);
							job->sourceData = AddRef(sourceData);
							m_pendingJobs.push(AddRef(job));

							TRACE_SPAM("GL: Created pending async copy image job for '{}', size {}", sourceData->debugLabel(), range.buffer.size);
						}
					}
				}
			}

			tryStartPendingJobs();

			return true;
		}

		void DeviceCopyQueue::update(Frame* frame)
		{
			finishCompletedJobs(frame);
			tryStartPendingJobs();
		}

		void DeviceCopyQueue::stop()
		{
			base::ScopeTimer timer;

			// remove any jobs from the list of jobs
			{
				auto lock = CreateLock(m_pendingJobLock);

				if (!m_pendingJobs.empty())
				{
					TRACE_INFO("GL: There are {} scheduled copy jogs, canceling", m_pendingJobs.size());
					m_pendingJobs.clear();
				}
			}

			// wait for existing jobs to finish
			while (1)
			{
				{
					auto lock = CreateLock(m_processingJobLock);
					if (m_processingJobs.empty())
						break;

					TRACE_INFO("GL: There are still {} jobs in processing list", m_processingJobs.size());

					for (const auto& job : m_processingJobs)
						job->flagCanceled.exchange(true); // prevent length operation if possible

					base::Sleep(100);
				}
			}

			TRACE_INFO("GL: Copy queue purged in {}", timer);
		}
	

		void DeviceCopyQueue::tryStartPendingJobs()
		{
			auto lock = CreateLock(m_pendingJobLock);
			uint32_t numStaredJobs = 0;
			while (!m_pendingJobs.empty())
			{
				auto job = m_pendingJobs.top();

				// try to allocate the staging buffer
				if (auto stagingArea = m_pool->allocate(job->stagingAreaSize, job->sourceData->debugLabel()))
				{
					job->stagingArea = stagingArea;
					m_pendingJobs.pop();

					// move to "processing list"
					{
						auto lock2 = CreateLock(m_processingJobLock);
						m_processingJobs.pushBack(job);
						numStaredJobs += 1;
					}

					// create the fiber that will handle copying the data to the staging area
					// NOTE: this can happen outside of render thread as it's basically a memcpy
					RunFiber("AsyncCopy") << [job](FIBER_FUNC)
					{
						if (!job->flagCanceled)
						{
							PC_SCOPE_LVL2(SourceDataWrite);
							job->sourceData->writeSourceData(job->stagingArea.dataPtr, job->stagingAreaSize, job->range);
							job->flagSourceDataCopyFinished = true;
						}
					};
				}
				else
				{
					TRACE_SPAM("GL: still no memory to start more jobs, {} pending, {} processing, {} staging used in {} blocks",
						m_pendingJobs.size(), m_processingJobs.size(), MemSize(m_pool->numAllocatedBytes()), m_pool->numAllocatedBlocks());
				}
			}

			if (numStaredJobs > 0)
			{
				TRACE_SPAM("GL: started {} async copy jobs, {} pending, {} processing, {} staging used in {} blocks",
					numStaredJobs, m_pendingJobs.size(), m_processingJobs.size(), MemSize(m_pool->numAllocatedBytes()), m_pool->numAllocatedBlocks());
			}
		}

		void DeviceCopyQueue::Job::print(base::IFormatStream& f) const
		{
			f << id;
			f << " form '";
			f << sourceData->debugLabel();
			f << "'";
		}

		void DeviceCopyQueue::finishCompletedJobs(Frame* frame)
		{
			// look at the job list and extract ones completed
			{
				m_tempJobList.reset();

				auto lock = CreateLock(m_processingJobLock);
				for (auto i : m_processingJobs.indexRange().reversed())
				{
					const auto& job = m_processingJobs[i];
					if (job->flagSourceDataCopyFinished.load())
					{
						TRACE_SPAM("Discovered finished job '{}'", *job);

						m_tempJobList.pushBack(job);
						m_processingJobs.eraseUnordered(i);
					}
				}
			}

			// finally issue copies to target resources (if they still exist that is as they might have been deleted during the sourceData pulling)
			for (const auto& job : m_tempJobList)
			{
				// find target object
				if (auto* obj = m_objects->resolveStatic(job->id, ObjectType::Invalid))
				{
					// flush any writes to make them device visible
					const auto stagingBufferView = m_pool->flushWrites(job->stagingArea);

					// push data to object
					if (obj->objectType() == ObjectType::Buffer)
					{
						auto* buffer = static_cast<Buffer*>(obj);
						buffer->copyFromBuffer(stagingBufferView, job->range);
					}
					else if (obj->objectType() == ObjectType::Image)
					{
						auto* image = static_cast<Image*>(obj);
						image->copyFromBuffer(stagingBufferView, job->range);
					}

					// make sure the staging area is reused at the end of current frame
					auto area = job->stagingArea;
					frame->registerCompletionCallback([this, area]()
						{
							m_pool->free(area);
						});
				}
				else
				{
					// object lost
					TRACE_WARNING("GL: Target object jof async copy job {} lost before copy finished", *job);
				}

				// signal fence now to indicate that we copied data to the GPU side
				Fibers::GetInstance().signalCounter(job->fence, 1);					
			}
		}

		//--

    } // gl4
} // rendering
