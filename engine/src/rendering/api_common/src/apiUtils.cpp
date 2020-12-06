/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiUtils.h"
#include "rendering/device/include/renderingDeviceApi.h"

namespace rendering
{
    namespace api
    {
		//--

		FrameCompleteionQueue::FrameCompleteionQueue()
			: m_currentFrameIndex(0)
		{
		}

		FrameCompleteionQueue::~FrameCompleteionQueue()
		{
			for (const auto& table : m_callbacks)
				ASSERT_EX(table.empty(), "There are callbacks registered for future that will never happen - flushing didn't work");
		}

		bool FrameCompleteionQueue::registerNotification(uint64_t frameIndex, IDeviceCompletionCallback* callback)
		{
			auto lock = CreateLock(m_lock);

			DEBUG_CHECK_RETURN_EX_V(frameIndex >= m_currentFrameIndex, "Cannot register callback for something in the past", false);

			auto leadIndex = frameIndex - m_currentFrameIndex;
			DEBUG_CHECK_RETURN_EX_V(leadIndex < MAX_LEAD_COUNTER, "Cannot register callback for something so much in the future. Is one part of the pipeline stalling?", false);

			m_callbacks[leadIndex].pushBack(AddRef(callback));
			return true;
		}

		class StdFunctionCompletionCallback : public IDeviceCompletionCallback
		{
		public:
			StdFunctionCompletionCallback(const std::function<void(void)>& func)
				: m_func(func)
			{}

			void signalCompletion() override final
			{
				m_func();
			}

		private:
			std::function<void(void)> m_func;
		};

		bool FrameCompleteionQueue::registerNotification(uint64_t frameIndex, const std::function<void(void)>& func)
		{
			auto callback = base::RefNew<StdFunctionCompletionCallback>(func);
			return registerNotification(frameIndex, callback);
		}

		void FrameCompleteionQueue::signalNotifications(uint64_t newFrameIndex)
		{
			PC_SCOPE_LVL0(FrameCompletionCallbacks);

			base::Array<DeviceCompletionCallbackPtr> callList[MAX_LEAD_COUNTER];

			{
				auto lock = CreateLock(m_lock);
				DEBUG_CHECK_RETURN_EX(newFrameIndex > m_currentFrameIndex, "Frame index must be monotonic");

				auto count = newFrameIndex - m_currentFrameIndex; // how many frames have passed ?
				if (count > 1)
				{
					TRACE_SPAM("Frame skip detected of {} frames: {} -> {}", count, m_currentFrameIndex, newFrameIndex);
				}
				else if (count > 2)
				{
					TRACE_INFO("Frame skip detected of {} frames: {} -> {}", count, m_currentFrameIndex, newFrameIndex);
				}
				else if (count > 3)
				{
					TRACE_WARNING("Frame skip detected of {} frames: {} -> {}", count, m_currentFrameIndex, newFrameIndex);
				}

				// extract data to call
				for (uint32_t i = 0; i < count; ++i)
					callList[i] = std::move(m_callbacks[i]); // moves [0] out

				// shift rest of data
				for (uint32_t i = count; i < MAX_LEAD_COUNTER; ++i)
					m_callbacks[i - count] = std::move(m_callbacks[i]); // moves int [1-1] = [0] content of [1]

				// set new frame
				m_currentFrameIndex = newFrameIndex;
			}

			// call callbacks outside the lock and in order
			for (auto& table : callList)
			{
				for (auto& callback : table)
				{
					callback->signalCompletion();
					callback.reset();
				}

				table.clear();
			}

			// TODO: try to reuse memory if nothing by putting empty tables back to use (if nothing else was added in the mean time)
		}

		//--

    } // api
} // rendering
