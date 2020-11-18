/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "glObject.h"
#include "glDevice.h"
#include "glDeviceThread.h"
#include "glExecutor.h"
#include "glTempBuffer.h"
#include "glFrame.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingCommands.h"
#include "rendering/device/include/renderingDeviceApi.h"

#ifdef PLATFORM_WINAPI
#include <Windows.h>
#elif defined(PLATFORM_LINUX)
#include <dlfcn.h>
#endif

#define USE_RENDER_DOC

#ifdef USE_RENDER_DOC
#include "renderdoc_api.h"
#endif

namespace rendering
{
    namespace gl4
    {

        //--

        Frame::Frame(DeviceThread* thread)
            : m_numDeclaredFrames(0)
            , m_numRecordedFrames(0)
            , m_thread(thread)
        {
            m_pendingFences.reserve(4);
        }

        Frame::~Frame()
        {
            ASSERT_EX(m_pendingFences.empty(), "Deleting sequence with unfinished fences");

            // call completion callbacks
            for (auto callback  : m_completionCallbacks)
                (*callback)();
            m_completionCallbacks.clearPtr();

            // delete object
            if (!m_deferedDeletionObjects.empty())
            {
                TRACE_INFO("Has {} objects to delete", m_deferedDeletionObjects.size());
                for (auto obj : m_deferedDeletionObjects)
                    delete obj;
            }
        }

        void Frame::attachPendingFrame()
        {
            m_numDeclaredFrames += 1;
        }

        void Frame::attachRecordedFrame(GLsync sync)
        {
            auto lock = base::CreateLock(m_pendingFencesLock);

            m_numRecordedFrames += 1;
            m_pendingFences.pushBack(sync);
            ASSERT_EX(m_numRecordedFrames <= m_numDeclaredFrames, "More fences than declared frames");
            ASSERT_EX(m_pendingFences.size() <= m_numRecordedFrames, "More fences than declared frames");
        }

        bool Frame::checkFences()
        {
            // take the lock
            auto lock = base::CreateLock(m_pendingFencesLock);

            // check if the fence has completed
            for (int j = m_pendingFences.lastValidIndex(); j >= 0; --j)
            {
                auto fence = m_pendingFences[j];
                auto ret = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);

                if (ret == GL_ALREADY_SIGNALED || ret == GL_CONDITION_SATISFIED)
                {
                    // fence was completed, remove it from the list
                    m_pendingFences.removeUnordered(fence);
                    glDeleteSync(fence);
                }
                else if (ret == GL_TIMEOUT_EXPIRED)
                {
                    // fence was not yet completed
                }
                else
                {
                    // something failed, warn and remove the fence to prevent deadlocks
                    auto error = glGetError();
                    TRACE_WARNING("Frame fence failed with return code {} and error code {}", ret, error);
                    m_pendingFences.removeUnordered(fence);
                    glDeleteSync(fence);
                } 
            }

            // return true if all fences were completed
            return m_pendingFences.empty() && (m_numDeclaredFrames == m_numRecordedFrames);
        }

        void Frame::registerObjectForDeletion(Object* obj)
        {
            if (obj)
            {
                auto lock = base::CreateLock(m_completionCallbacksLock);
                m_deferedDeletionObjects.pushBack(obj);
            }
        }

        void Frame::registerCompletionCallback(FrameCompletionCallback callback)
        {
            if (callback)
            {
                auto lock = base::CreateLock(m_completionCallbacksLock);

                auto callBackCopy = new FrameCompletionCallback(std::move(callback));
                m_completionCallbacks.emplaceBack(callBackCopy);
            }
        }

        //--

                //---

#ifdef USE_RENDER_DOC

        class RenderDocCapture : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_RENDERING_FRAME)

        public:
            RenderDocCapture()
                : m_api(nullptr)
            {
#ifdef PLATFORM_WINAPI
                if (HMODULE mod = (HMODULE)LoadLibraryW(L"renderdoc.dll"))
                {
                    pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
                    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&m_api);
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
#elif defined(PLATFORM_LINUX)
                if (void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD))
                {
                    pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
                    int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&m_api);
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

        static void CollectCommandBuffers(command::CommandBuffer* commandBuffer, base::Array<command::CommandBuffer*>& outCommandBuffers)
        {
            commandBuffer->visitHierarchy([&outCommandBuffers](command::CommandBuffer* ptr)
                {
                    outCommandBuffers.pushBack(ptr);
                    return false;
                });
        }

        void BuildTransientData(Device* drv, Frame* seq, PerformanceStats& outStats, command::CommandBuffer* commandBuffer, exec::RuntimeDataAllocations& outData)
        {
            PC_SCOPE_LVL2(BuildTransientData);

            // timer
            base::ScopeTimer timer;

            // collect all referenced command buffers
            base::InplaceArray<command::CommandBuffer*, 100> allCommandBuffers;
            CollectCommandBuffers(commandBuffer, allCommandBuffers);

            {
                PC_SCOPE_LVL2(Collect);

                // report constant usage
                for (auto& commandBuffer : allCommandBuffers)
                {
                    if (commandBuffer->gatheredState().totalConstantsUploadSize > 0)
                    {
                        outData.reportConstantsBlockSize(commandBuffer->gatheredState().totalConstantsUploadSize);
                        auto prev = commandBuffer->gatheredState().constantUploadHead;
                        for (auto cur = commandBuffer->gatheredState().constantUploadHead; cur; cur = cur->nextConstants)
                        {
                            uint32_t mergedOffset = 0;

                            const void* constData = cur->dataPtr;
                            outData.reportConstData(cur->offset, cur->dataSize, constData, mergedOffset);
                            cur->mergedRuntimeOffset = mergedOffset;
                            prev = cur;

                            outStats.uploadTotalSize += cur->dataSize;
                            outStats.uploadConstantsCount += 1;
                            outStats.uploadConstantsSize += cur->dataSize;
                        }
                    }
                }

                // report and prepare transient buffers
                for (auto& commandBuffer : allCommandBuffers)
                {
                    /*for (auto cur = commandBuffer->gatheredState().triansienBufferAllocHead; cur; cur = cur->next)
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
                    }*/
                }

                // report the buffer updates
                for (auto& commandBuffer : allCommandBuffers)
                {
                    for (auto cur = commandBuffer->gatheredState().dynamicBufferUpdatesHead; cur; cur = cur->next)
                    {
                        outData.reportBufferUpdate(cur->dataBlockPtr, cur->dataBlockSize, cur->stagingBufferOffset);

                        if (cur->dataBlockSize > 0)
                        {
                            outStats.uploadTotalSize += cur->dataBlockSize;
                            outStats.uploadDynamicBufferCount += 1;
                            outStats.uploadDynamicBufferSize += cur->dataBlockSize;
                        }
                    }
                }
            }

            // update stats
            outStats.uploadTime = timer.timeElapsed();
        }

        static base::UniquePtr<RenderDocCapture> ConditionalStartCapture(command::CommandBuffer* masterCommandBuffer)
        {
#ifndef BUILD_RELEASE
            auto* cmd = masterCommandBuffer->commands();
            while (cmd)
            {
                if (cmd->op == command::CommandCode::TriggerCapture)
                {
                    return base::CreateUniquePtr<RenderDocCapture>();
                }
                else if (cmd->op == command::CommandCode::ChildBuffer)
                {
                    auto* op = static_cast<const command::OpChildBuffer*>(cmd);
                    if (auto ret = ConditionalStartCapture(op->childBuffer))
                        return ret;
                }

                cmd = command::GetNextCommand(cmd);
            }
#endif

            return nullptr;
        }

        //---

        void ExecuteCommandsFromBuffer(Device* drv, DeviceThread* thread, Frame* seq, exec::FrameExecutor& stateTracker, command::CommandBuffer* commandBuffer)
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
#include "rendering/device/include/renderingCommandOpcodes.inl"
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
                stateTracker.stats().numLogicalCommandBuffers += 1;
                stateTracker.stats().numCommands += 1;
            }
        }

        void ExecuteCommands(Device* drv, DeviceThread* thread, Frame* seq, PerformanceStats* stats, command::CommandBuffer* masterCommandBuffer)
        {
            PC_SCOPE_LVL1(ExecuteCommands);
            base::ScopeTimer totalTimer;

            // capture if requested
            auto capture = ConditionalStartCapture(masterCommandBuffer);

            // build the transient data for the frame
            exec::RuntimeDataAllocations frameData;
            BuildTransientData(drv, seq, *stats, masterCommandBuffer, frameData);
            
            // measure total GPU rendering time
            auto queryId = thread->createQuery();
            GL_PROTECT(glBeginQuery(GL_TIME_ELAPSED, queryId));

            // in OpenGL we produce only one native command buffer
            stats->numNativeCommandBuffers += 1;

            // execute (recursively) the command buffer
            {
                exec::FrameExecutor stateTracker(drv, thread, seq, frameData, stats);
                ExecuteCommandsFromBuffer(drv, thread, seq, stateTracker, masterCommandBuffer);
            }

            // mark end
            GL_PROTECT(glEndQuery(GL_TIME_ELAPSED));
            stats->totalCPUTime = totalTimer.timeElapsed();

            // once the frame ends make a stat block out of this one
            // NOTE: this prevents stat block from being reported as ready as long as the frame has not completed
            auto statsRef = base::RefPtr<PerformanceStats>(AddRef(stats));
            seq->registerCompletionCallback([queryId, thread, statsRef]() mutable
                {
                    int done = 0;
                    GL_PROTECT(glGetQueryObjectiv(queryId, GL_QUERY_RESULT_AVAILABLE, &done));
                    if (done)
                    {
                        GLuint64 value = 0; // will be in nano seconds
                        GL_PROTECT(glGetQueryObjectui64v(queryId, GL_QUERY_RESULT, &value));
                        statsRef->totalGPUTime = (double)value / (double)1000000000.0;
                    }

                    thread->releaseQuery(queryId);
                });
        }

        //--

    } // gl4
} // device
