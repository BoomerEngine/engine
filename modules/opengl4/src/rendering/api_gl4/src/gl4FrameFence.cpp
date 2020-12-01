/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4FrameFence.h"

namespace rendering
{
	namespace api
	{
		namespace gl4
		{

			//---

			FrameFence::FrameFence()
			{
				m_signaled = false;
				m_issueTime.resetToNow();

				m_glFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
				DEBUG_CHECK_EX(m_glFence != nullptr, "Fence not created");
			}

			FrameFence::~FrameFence()
			{
				DEBUG_CHECK_EX(m_signaled, "Deleting unsignalled frame fence");

				if (m_glFence)
				{
					glDeleteSync(m_glFence);
					m_glFence = 0;
				}
			}

			FrameFence::FenceResult FrameFence::check()
			{
				DEBUG_CHECK_RETURN_EX_V(!m_signaled, "Signaled fence not removed", FenceResult::Completed);

				auto ret = glClientWaitSync(m_glFence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
				switch (ret)
				{
					case GL_ALREADY_SIGNALED: 
					case GL_CONDITION_SATISFIED:
						m_signaled = true;
						return FenceResult::Completed;

					case GL_TIMEOUT_EXPIRED:
						return FenceResult::Pending;

					default:
						break;
				}

				auto error = glGetError();
				TRACE_WARNING("Frame fence failed with return code {} and error code {}", ret, error);
				m_signaled = true;
				return FenceResult::Failed;
			}

			//---

		} // gl4
	} // api
} // rendering
