/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiFrameFence.h"

namespace rendering
{
	namespace api
	{
		namespace nul
		{

			//---

			FrameFence::FrameFence(float timeout)
			{
				m_signaled = false;
				m_expirationTime = base::NativeTimePoint::Now() + (double)timeout;
			}

			FrameFence::~FrameFence()
			{
				DEBUG_CHECK_EX(m_signaled, "Deleting unsignalled frame fence");
			}

			FrameFence::FenceResult FrameFence::check()
			{
				DEBUG_CHECK_EX(!m_signaled, "Signaled fence not removed");

				if (m_expirationTime.reached())
				{
					m_signaled = true;
					return FenceResult::Completed;
				}

				return FenceResult::Pending;
			}

			//---

		} // nul
	} // api
} // rendering
