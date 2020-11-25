/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\frame\execution #]
***/

#include "build.h"
#include "glExecutor.h"
#include "glDevice.h"
#include "glDeviceThread.h"
#include "glUtils.h"
#include "glObjectCache.h"

#include "base/system/include/timing.h"
#include "base/image/include/image.h"
#include "base/image/include/imageUtils.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {

            //---

            FrameExecutor::FrameExecutor(Device* drv, DeviceThread* thread, Frame* frame, const RuntimeDataAllocations& data, PerformanceStats* stats)
                : m_device(*drv)
                , m_thread(*thread)
                , m_frame(*frame)
                , m_objectCache(drv->objectCache())
                , m_objectRegistry(drv->objectRegistry())
                , m_stats(stats)
            {
                auto defaultState = RenderStates();
                defaultState.apply(RenderDirtyStateTrack::ALL_STATES());

                prepareFrameData(data);
            }

            FrameExecutor::~FrameExecutor()
            {
                resetCommandBufferRenderState();

                

                if (m_pass.fbo != 0)
                {
                    GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, 0));
                    GL_PROTECT(glDeleteFramebuffers(1, &m_pass.fbo));
                    m_pass.fbo = 0;

                    base::mem::PoolStats::GetInstance().notifyFree(POOL_API_FRAMEBUFFERS, 1);
                }
            }


            void FrameExecutor::runBeginBlock(const command::OpBeginBlock& op)
            {
                GL_PROTECT(glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, m_nextDebugMessageID++, -1, op.payload<char>()));
            }

            void FrameExecutor::runEndBlock(const command::OpEndBlock& op)
            {
                GL_PROTECT(glPopDebugGroup());
            }


            //---

            
            //--

        } // exec
    } // gl4
} // rendering
