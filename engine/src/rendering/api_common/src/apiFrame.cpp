/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiObject.h"
#include "apiThread.h"
#include "apiFrame.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingCommands.h"
#include "rendering/device/include/renderingDeviceApi.h"

#ifdef PLATFORM_WINAPI
#include <Windows.h>
#elif defined(PLATFORM_LINUX)
#include <dlfcn.h>
#endif

#ifndef BUILD_FINAL
	#define USE_RENDER_DOC
#endif

#ifdef USE_RENDER_DOC
	#include "renderdoc_api.h"
#endif

namespace rendering
{
    namespace api
    {
		//--

		IBaseFrameFence::~IBaseFrameFence()
		{}

        //--

        Frame::Frame(IBaseThread* thread, uint32_t frameIndex)
            : m_numDeclaredFrames(0)
            , m_numRecordedFrames(0)
			, m_frameIndex(frameIndex)
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

        void Frame::attachRecordedFrame(IBaseFrameFence* fence)
        {
			ASSERT(fence);

            auto lock = base::CreateLock(m_pendingFencesLock);

            m_numRecordedFrames += 1;
            m_pendingFences.pushBack(fence);
            ASSERT_EX(m_numRecordedFrames <= m_numDeclaredFrames, "More fences than declared frames");
            ASSERT_EX(m_pendingFences.size() <= m_numRecordedFrames, "More fences than declared frames");
        }

        bool Frame::checkFences()
        {
            auto lock = base::CreateLock(m_pendingFencesLock);

            // check if the fence has completed
            for (int j = m_pendingFences.lastValidIndex(); j >= 0; --j)
            {
                auto fence = m_pendingFences[j];
				auto ret = fence->check();

                if (ret != IBaseFrameFence::FenceResult::Pending)
                {
					// error ?
					if (ret != IBaseFrameFence::FenceResult::Completed)
						TRACE_WARNING("Fence for frame '{}' failed", m_frameIndex);

                    // fence was completed, remove it from the list
                    m_pendingFences.removeUnordered(fence);
                    delete fence;
                }
            }

            // return true if all fences were completed, this signals posibility of deleting this frame
            return m_pendingFences.empty() && (m_numDeclaredFrames == m_numRecordedFrames);
        }

        void Frame::registerObjectForDeletion(IBaseObject* obj)
        {
			DEBUG_CHECK_RETURN(obj && obj->canDelete());

            auto lock = base::CreateLock(m_completionCallbacksLock);
            m_deferedDeletionObjects.pushBack(obj);
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

    } // gl4
} // rendering
