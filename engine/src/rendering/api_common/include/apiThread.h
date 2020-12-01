/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "base/containers/include/hashSet.h"
#include "base/containers/include/queue.h"
#include "base/system/include/spinLock.h"
#include "base/system/include/thread.h"
#include "base/system/include/semaphoreCounter.h"
#include "base/fibers/include/fiberSystem.h"
#include "base/fibers/include/fiberSyncPoint.h"

#include "rendering/device/include/renderingOutput.h"
#include "rendering/device/include/renderingDeviceApi.h"

namespace rendering
{
	namespace api
	{
		//---

		// a general API-side device thread, all things happen here
		class RENDERING_API_COMMON_API IBaseThread : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_API_RUNTIME)

		public:
			IBaseThread(IBaseDevice* drv, WindowManager* windows);
			virtual ~IBaseThread();

			//--

			// pool for staging copy data
			INLINE IBaseStagingPool* copyPool() const { return m_copyPool; }

			// async copy queue
			INLINE IBaseCopyQueue* copyQueue() const { return m_copyQueue; }
			
			// object registry - contains all public objects indexed via their handle (ObjectID)
			INLINE ObjectRegistry* objectRegistry() const { return m_objectRegistry; }

			// object registry - contains internal API objects
			INLINE IBaseObjectCache* objectCache() const { return m_objectCache; }

			// pool for staging buffers
			INLINE IBaseTransientBufferPool* transientStagingPool() const { return m_transientStagingPool; }

			// pool for constant buffers
			INLINE IBaseTransientBufferPool* transientConstantPool() const { return m_transientConstantPool; }

			//--

			// start device thread, initializes API
			bool startThread(const base::app::CommandLine& cmdLine);

			// finish device thread, closes API
			void stopThread();

			//--

			// API object is no longer needed, add it to current frame as an object to delete once frame finished
			void scheduleObjectForDestruction(IBaseObject* ptr);

			//--

			// stop all driver side process
			void sync();

			// called from client to indicate that a logical new frame has started
			void advanceFrame();

			// run a custom function in the thread context
			void run(const std::function<void()>& func);

			// submit command buffers for execution
			void submit(command::CommandBuffer* masterCommandBuffer);

			//--

			// create swapchain for specific output 
			// NOTE: this is OS/third party specific, can be called from any thread but can be internally synchronized if needed
			virtual IBaseSwapchain* createOptimalSwapchain(const OutputInitInfo& info) = 0;

			// create frame fence, fence can be used to check if current frame has finished
			virtual IBaseFrameFence* createOptimalFrameFence() = 0;

			// create buffer data object
			virtual IBaseBuffer* createOptimalBuffer(const BufferCreationInfo& info) = 0;

			// create image data object
			virtual IBaseImage* createOptimalImage(const ImageCreationInfo& info) = 0;

			// create sampler data 
			virtual IBaseSampler* createOptimalSampler(const SamplerState& state) = 0;

			// create shaders
			virtual IBaseShaders* createOptimalShaders(const ShaderData* data) = 0;

			// create pass layout
			virtual IBaseGraphicsPassLayout* createOptimalPassLayout(const GraphicsPassLayoutSetup& info) = 0;

			//-----

			// sync GPU (stop all work), usually inserts a GPU fence and waits for it
			// NOTE: called from render thread
			virtual void syncGPU_Thread() = 0;

			// record native command buffers
			// NOTE: called from render thread
			virtual void execute_Thread(Frame& frame, PerformanceStats& stats, command::CommandBuffer* masterCommandBuffer, RuntimeDataAllocations& data) = 0;;

			//--

		protected:
			volatile bool m_requestExit = false;

			base::Thread m_thread;
			uint32_t m_threadId = 0;

			base::SpinLock m_sequenceLock;
			base::Array<Frame*> m_sequencePendingList;

			Frame* m_currentFrame;
			std::atomic<uint32_t> m_currentFrameIndex;

			IBaseDevice* m_device = nullptr;
			WindowManager* m_windows = nullptr;

			base::RefPtr<ObjectRegistry> m_objectRegistry = nullptr;

			IBaseStagingPool* m_copyPool = nullptr;
			IBaseCopyQueue* m_copyQueue = nullptr;

			IBaseObjectCache* m_objectCache = nullptr;

			IBaseTransientBufferPool* m_transientStagingPool = nullptr;
			IBaseTransientBufferPool* m_transientConstantPool = nullptr;

			base::fibers::SyncPoint m_cleanupSync;
			//base::Array<GLuint> m_freeQueryObjects;

			//--

			base::Array<uint64_t> m_windowsToClose;
			base::Mutex m_windowsToCloseLock;

			//--

			struct Job : public base::NoCopy
			{
				RTTI_DECLARE_POOL(POOL_API_RUNTIME)

			public:
				std::function<void()> m_jobFunc;
			};

			base::Queue<Job*> m_jobQueue;
			base::SpinLock m_jobQueueLock;
			base::Semaphore m_jobQueueSemaphore;

			Job* popJob();
			void pushJob(const std::function<void()>& func);

			//--

			virtual IBaseStagingPool* createOptimalStagingPool(uint32_t size, uint32_t pageSize, const base::app::CommandLine& cmdLine) = 0;
			virtual IBaseCopyQueue* createOptimalCopyQueue(const base::app::CommandLine& cmdLine) = 0;
			virtual IBaseObjectCache* createOptimalObjectCache(const base::app::CommandLine& cmdLine) = 0;

			virtual IBaseTransientBufferPool* createOptimalTransientStagingPool(const base::app::CommandLine& cmdLine) = 0;
			virtual IBaseTransientBufferPool* createOptimalTransientConstantPool(const base::app::CommandLine& cmdLine) = 0;

			virtual ObjectRegistry* createOptimalObjectRegistry(const base::app::CommandLine& cmdLine);

			virtual bool threadStartup(const base::app::CommandLine& cmdLine); // called on thread to initialize API
			virtual void threadFinish(); // called on thread to initialize API

			void threadFunc();

			void closeFinishedFrames_Thread();

			friend class Frame;
		};

		//---

	} // api
} // rendering