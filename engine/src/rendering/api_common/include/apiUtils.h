/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "base/system/include/spinLock.h"

namespace rendering
{
    namespace api
    {

		//---

		// indexed completion list - clients can register for particular number 
		// and when host "updates" the version all subscribed numbers are called
		class RENDERING_API_COMMON_API FrameCompleteionQueue : public base::NoCopy
		{
		public:
			FrameCompleteionQueue();
			~FrameCompleteionQueue(); // must be empty when destroyed

			//--

			// current fame counter
			INLINE uint64_t frameIndex() const { return m_currentFrameIndex; }

			//--

			// signal all waiting notifications
			void signalNotifications(uint64_t newFrameIndex);

			// register for expiration of frame N, will be called once we reached AT LEAST frame N+1 (but we can skip ahead)
			// NOTE: it's impossible to register notification to expired frame - only to current and future ones
			bool registerNotification(uint64_t frameIndex, IDeviceCompletionCallback* callback);

			// register a function base notification
			bool registerNotification(uint64_t frameIndex, const std::function<void(void)>& func);

			//--

		private:
			static const auto MAX_LEAD_COUNTER = 8; // we can register up to this frames in the future

			base::SpinLock m_lock;

			uint64_t m_currentFrameIndex = 0;

			base::Array<DeviceCompletionCallbackPtr> m_callbacks[MAX_LEAD_COUNTER]; // list modulo N
		};

		//---

    } // gl4
} // rendering