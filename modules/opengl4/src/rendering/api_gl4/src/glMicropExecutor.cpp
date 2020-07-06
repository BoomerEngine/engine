/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\execution #]
***/

#include "build.h"
#include "glMicropExecutor.h"
#include "glMicropExecutorState.h"
#include "glTransientAllocator.h"
#include "glDriverThread.h"
#include "glDriver.h"

#ifdef PLATFORM_WINAPI
#elif defined(PLATFORM_LINUX)
#include <dlfcn.h>
#endif

#define USE_RENDER_DOC

#ifdef USE_RENDER_DOC
    #include "renderdoc_api.h"
#endif

#include "rendering/driver/include/renderingCommandBuffer.h"
#include "rendering/driver/include/renderingCommands.h"
#include "rendering/driver/include/renderingDriver.h"

namespace rendering
{
    namespace gl4
    {

        //---

        #ifdef USE_RENDER_DOC

        class RenderDocCapture
        {
        public:
            RenderDocCapture()
                : m_api(nullptr)
            {
#ifdef PLATFORM_WINAPI
              
#elif defined(PLATFORM_LINUX)
				if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
				{
					pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
					int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)& m_api);
					if (ret == 1)
					{
						TRACE_INFO("Attached to RenderDOC");

						if (m_api)
							m_api->StartFrameCapture(NULL, NULL);
					}
					else
					{
						TRACE_WARNING("Failed to attach to RenderDOC");
					}
				}
				else
				{
					TRACE_WARNING("No RenderDOC to attach to");
				}
#endif
            }

            ~RenderDocCapture()
            {
                if (m_api)
                {
                    m_api->EndFrameCapture(NULL, NULL);
                    TRACE_INFO("Finished RenderDOC capture");
                }
            }

        private:
            RENDERDOC_API_1_1_2* m_api;
        };

        #else

        class RenderDocCapture
        {
        public:
            RenderDocCapture() {};
            ~RenderDocCapture() {};
        };

        #endif


        //---

        static bool CanRunDuringFatalErrors(const command::CommandCode op)
        {
            switch (op)
            {
            case command::CommandCode::WaitForCounter:
            case command::CommandCode::SignalCounter:
                return true;
            }

            return false;
        }

        //---

        static void CollectCommandBuffers(command::CommandBuffer* commandBuffer, base::Array<command::CommandBuffer*>& outCommandBuffers)
        {
            commandBuffer->visitHierarchy([&outCommandBuffers](command::CommandBuffer* ptr)
                {
                    outCommandBuffers.pushBack(ptr);
                    return false;
                });
        }

        TransientFrame* BuildTransientData(Driver* drv, DriverFrame* seq, DriverPerformanceStats& outStats, command::CommandBuffer* commandBuffer)
        {
            PC_SCOPE_LVL2(BuildTransientData);

            // timer
            base::ScopeTimer timer;

            // collect all referenced command buffers
            base::InplaceArray<command::CommandBuffer*, 100> allCommandBuffers;
            CollectCommandBuffers(commandBuffer, allCommandBuffers);

            // prepare transient builder
            TransientFrameBuilder transientFrameBuilder;

            {
                PC_SCOPE_LVL2(Collect);

                // report constant usage
                for (auto &commandBuffer : allCommandBuffers)
                {
                    if (commandBuffer->gatheredState().totalConstantsUploadSize > 0)
                    {
                        transientFrameBuilder.reportConstantsBlockSize(commandBuffer->gatheredState().totalConstantsUploadSize);
                        auto prev  = commandBuffer->gatheredState().constantUploadHead;
                        for (auto cur = commandBuffer->gatheredState().constantUploadHead; cur; cur = cur->nextConstants)
                        {
                            uint32_t mergedOffset = 0;

                            const void* constData = cur->dataPtr;
                            transientFrameBuilder.reportConstData(cur->offset, cur->dataSize, constData, mergedOffset);
                            cur->mergedRuntimeOffset = mergedOffset;
                            prev = cur;

                            outStats.m_uploadTotalSize += cur->dataSize;
                            outStats.m_uploadConstantsCount += 1;
                            outStats.m_uploadConstantsSize += cur->dataSize;
                        }
                    }
                }

                // report and prepare transient buffers
                for (auto &commandBuffer : allCommandBuffers)
                {
                    for (auto cur = commandBuffer->gatheredState().triansienBufferAllocHead; cur; cur = cur->next)
                    {
                        // allocate the resident storage for the buffer on the GPU
                        auto initialData = (cur->initializationDataSize != 0) ? cur->initializationData : nullptr;
                        transientFrameBuilder.reportBuffer(cur->buffer, initialData, cur->initializationDataSize);

                        if (cur->initializationDataSize > 0)
                        {
                            outStats.m_uploadTotalSize += cur->initializationDataSize;
                            outStats.m_uploadTransientBufferCount += 1;
                            outStats.m_uploadTransientBufferSize += cur->initializationDataSize;
                        }
                    }
                }

                // report the buffer updates
                for (auto &commandBuffer : allCommandBuffers)
                {
                    for (auto cur = commandBuffer->gatheredState().dynamicBufferUpdatesHead; cur; cur = cur->next)
                    {
                        transientFrameBuilder.reportBufferUpdate(cur->dataBlockPtr, cur->dataBlockSize, cur->stagingBufferOffset);

                        if (cur->dataBlockSize > 0)
                        {
                            outStats.m_uploadTotalSize += cur->dataBlockSize;
                            outStats.m_uploadDynamicBufferCount += 1;
                            outStats.m_uploadDynamicBufferSize += cur->dataBlockSize;
                        }
                    }
                }
            }

            // create a transient frame allocator for the
            auto frame  = drv->transientAllocator().buildFrame(transientFrameBuilder);
            if (!frame)
                return nullptr;

            // make sure the transient data is freed once the GPU stops using it
            seq->registerCompletionCallback([frame]()
                                            {
                                                MemDelete(frame);
                                            });

            // update stats
            outStats.m_uploadTime = timer.timeElapsed();
            return frame;
        }

        static base::UniquePtr<RenderDocCapture> ConditionalStartCapture(command::CommandBuffer* masterCommandBuffer)
        {
#ifndef BUILD_RELEASE
            /*for (auto& commandBuffer : commandBuffers)
            {
                auto* cmd = commandBuffer->commands();
                while (cmd)
                {
                    if (cmd->op == command::CommandCode::TriggerCapture)
                        return base::CreateUniquePtr<RenderDocCapture>();
                    cmd = GetNextCommand(cmd);
                }
            }*/
#endif

            return nullptr;
        }

        //---

        void ExecuteCommandsFromBuffer(Driver* drv, DriverThread* thread, DriverFrame* seq, ExecutorStateTracker& stateTracker, command::CommandBuffer* commandBuffer)
        {
            // process the commands
            base::ScopeTimer commandTimer;
            {
                PC_SCOPE_LVL2(ExecuteCommandBuffer);

                // process commands
                uint32_t numCommands = 0;
                auto* cmd = commandBuffer->commands();
                while (cmd)
                {
                    // child buffer
                    if (cmd->op == command::CommandCode::ChildBuffer)
                    {
                        auto* op = static_cast<const command::OpChildBuffer*>(cmd);

                        stateTracker.pushParamState(op->inheritsParameters);
                        ExecuteCommandsFromBuffer(drv, thread, seq, stateTracker, op->childBuffer);
                        stateTracker.popParamState();
                    }
                    else
                    {
                        // run only if we don't have errors
                        switch (cmd->op)
                        {
#define RENDER_COMMAND_OPCODE(x) case command::CommandCode::##x: { stateTracker.run##x(*static_cast<const command::Op##x*>(cmd)); break; }
#include "rendering/driver/include/renderingCommandOpcodes.inl"
#undef RENDER_COMMAND_OPCODE
                        default:
                            DEBUG_CHECK(!"Unsupported command recorded");
                            break;
                        }
                    }

                    cmd = command::GetNextCommand(cmd);
                    numCommands += 1;
                }

                // update stats
                stateTracker.stats().m_numLogicalCommandBuffers += 1;
                stateTracker.stats().m_numCommands += 1;
            }
        }

        void ExecuteCommands(Driver* drv, DriverThread* thread, DriverFrame* seq, DriverPerformanceStats* stats, command::CommandBuffer* masterCommandBuffer)
        {
            PC_SCOPE_LVL1(ExecuteCommands);
            base::ScopeTimer totalTimer;

            // capture if requested
            auto capture = ConditionalStartCapture(masterCommandBuffer);

            // build the transient data for the frame
            auto transientFrame  = BuildTransientData(drv, seq, *stats, masterCommandBuffer);
            if (!transientFrame)
            {
                TRACE_WARNING("To much transient data recorded into the frame for the rendering to cope with that, skipping frame as it cannot be rendered anyway");
                return;
            }

            // measure total GPU rendering time
            auto queryId = thread->createQuery();
            GL_PROTECT(glBeginQuery(GL_TIME_ELAPSED, queryId));

            // in OpenGL we produce only one native command buffer
            stats->m_numNativeCommandBuffers += 1;

            // execute (recursively) the command buffer
            {
                ExecutorStateTracker stateTracker(drv, thread, *transientFrame, stats);
                ExecuteCommandsFromBuffer(drv, thread, seq, stateTracker, masterCommandBuffer);
            }

            // mark end
            GL_PROTECT(glEndQuery(GL_TIME_ELAPSED));
            stats->m_totalCPUTime = totalTimer.timeElapsed();

            // once the frame ends make a stat block out of this one
            // NOTE: this prevents stat block from being reported as ready as long as the frame has not completed
            auto statsRef = base::RefPtr<DriverPerformanceStats>(AddRef(stats));
            seq->registerCompletionCallback([queryId, thread, statsRef]() mutable
            {
                int done = 0;
                GL_PROTECT(glGetQueryObjectiv(queryId, GL_QUERY_RESULT_AVAILABLE, &done));
                if (done)
                {
                    GLuint64 value = 0; // will be in nano seconds
                    GL_PROTECT(glGetQueryObjectui64v(queryId, GL_QUERY_RESULT, &value));
                    statsRef->m_totalGPUTime = (double)value / (double)1000000000.0;
                }

                thread->releaseQuery(queryId);
            });
        }

    } // gl4
} // driver
