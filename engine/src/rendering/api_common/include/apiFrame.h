/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

namespace rendering
{
    namespace api
    {
		//---

		// a "frame fence" to be signaled once frame finished
		class RENDERING_API_COMMON_API IBaseFrameFence : public base::NoCopy
		{
			RTTI_DECLARE_POOL(POOL_API_RUNTIME)

		public:
			virtual ~IBaseFrameFence();

			enum class FenceResult : uint8_t
			{
				Pending, // fence is still valid but has not yet completed
				Completed, // frame has finished
				Failed, // error, something happened
			};

			virtual FenceResult check() = 0;
		};

        //---

        // a frame in progress
        class RENDERING_API_COMMON_API Frame : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_RUNTIME)

        public:
            Frame(IBaseThread* thread, uint32_t frameIndex);
            ~Frame(); // calls all the callbacks

            //--

            // attach a frame
            void attachPendingFrame();

            // attach a recorded frame
            void attachRecordedFrame(IBaseFrameFence* fence);

            // look to see if this sequence has ended
            // NOTE: will return true if all of the fences in the sequence are completed
            bool checkFences();

            // register a completion callback
            void registerCompletionCallback(FrameCompletionCallback callback);

            // register an object for deferred deletion
            void registerObjectForDeletion(IBaseObject* obj);

            //--

        private:
            uint32_t m_numDeclaredFrames = 0;
            uint32_t m_numRecordedFrames = 0;

			uint32_t m_frameIndex;

            base::Array<IBaseFrameFence*> m_pendingFences;
            base::SpinLock m_pendingFencesLock;

            base::Array<FrameCompletionCallback*> m_completionCallbacks;
            base::SpinLock m_completionCallbacksLock;

            base::InplaceArray<IBaseObject*, 128> m_deferedDeletionObjects;
            base::SpinLock m_deferedDeletionObjectsLock;

            IBaseThread* m_thread;
        };

		//---

    } // api
} // rendering