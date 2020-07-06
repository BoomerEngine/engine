/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\execution #]
***/

#include "build.h"
#include "glMicropExecutorState.h"
#include "glDriver.h"
#include "glDriverThread.h"
#include "glUtils.h"
#include "glObjectCache.h"
#include "glTransientBufferAllocator.h"

#include "base/system/include/timing.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"

namespace rendering
{
    namespace gl4
    {

        //---

        ExecutorStateTracker::ExecutorStateTracker(Driver* drv, DriverThread* thread, TransientFrame& transientAllocatorFrame, DriverPerformanceStats* stats)
            : m_driver(drv)
            , m_thread(thread)
            , m_frame(transientAllocatorFrame)
            , m_objectCache(&drv->objectCache())
            , m_stats(stats)
        {
            auto defaultState = RenderStates();
            defaultState.apply(RenderDirtyStateTrack::ALL_STATES());
        }

        ExecutorStateTracker::~ExecutorStateTracker()
        {
            resetCommandBufferRenderState();

            DEBUG_CHECK_EX(m_activeSwapchains.empty(), "Some acquired outputs were not swapped!");

            if (m_pass.fbo != 0)
            {
                GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, 0));
                GL_PROTECT(glDeleteFramebuffers(1, &m_pass.fbo));
                m_pass.fbo = 0;

                base::mem::PoolStats::GetInstance().notifyFree(POOL_GL_FRAMEBUFFERS, 1);
            }
        }

        void ExecutorStateTracker::runAcquireOutput(const command::OpAcquireOutput& op)
        {
            bool canAquire = true;

            for (const auto& info : m_activeSwapchains)
            {
                DEBUG_CHECK_EX(info.swapchain != op.output, "Output is already acquired");
                if (info.swapchain == op.output)
                {
                    canAquire = false;
                    break;
                }
            }

            if (canAquire)
            {
                auto& info = m_activeSwapchains.emplaceBack();
                info.swapchain = op.output;
                info.colorRT = op.colorView.id();
                info.depthRT = op.depthView.id();
            }
        }

        void ExecutorStateTracker::runSwapOutput(const command::OpSwapOutput& op)
        {
            for (uint32_t i = 0; i < m_activeSwapchains.size(); ++i)
            {
                const auto& info = m_activeSwapchains[i];
                if (info.swapchain == op.output)
                {
                    if (op.swap)
                        m_thread->swapOutput(info.swapchain);

                    m_activeSwapchains.erase(i);
                    break;
                }
            }
        }

        void ExecutorStateTracker::runBeginBlock(const command::OpBeginBlock& op)
        {
            GL_PROTECT(glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, m_nextDebugMessageID++, -1, op.payload<char>()));
        }

        void ExecutorStateTracker::runEndBlock(const command::OpEndBlock& op)
        {
            GL_PROTECT(glPopDebugGroup());
        }

        void ExecutorStateTracker::runSignalCounter(const command::OpSignalCounter& op)
        {
            Fibers::GetInstance().signalCounter(op.counter, op.valueToSignal);
        }

        void ExecutorStateTracker::runWaitForCounter(const command::OpWaitForCounter& op)
        {
            Fibers::GetInstance().waitForCounterAndRelease(op.counter);
        }

        void ExecutorStateTracker::runNop(const command::OpNop& op)
        {
        }

        void ExecutorStateTracker::runHello(const command::OpHello& op)
        {
            
        }

        void ExecutorStateTracker::runNewBuffer(const command::OpNewBuffer& op)
        {
        }

        void ExecutorStateTracker::runTriggerCapture(const command::OpTriggerCapture& op)
        {
        }

        void ExecutorStateTracker::runChildBuffer(const command::OpChildBuffer& op)
        {
            ASSERT(!"Should not be called");
        }

        //---

        void ExecutorStateTracker::pushParamState(bool inheritCurrentParameters)
        {
            if (inheritCurrentParameters)
            {
                m_paramStateStack.emplaceBack(m_params);
            }
            else
            {
                m_paramStateStack.emplaceBack(std::move(m_params));
                m_params.paramBindings.reset();
                m_params.parameterBindingsChanged = true;
            }
        }

        void ExecutorStateTracker::popParamState()
        {
            DEBUG_CHECK_EX(!m_paramStateStack.empty(), "Param stack underflow");

            if (!m_paramStateStack.empty())
            {
                m_params = std::move(m_paramStateStack.back());
                m_params.parameterBindingsChanged = true;
                m_paramStateStack.popBack();
            }
        }

        //--

    } // gl4
} // driver
