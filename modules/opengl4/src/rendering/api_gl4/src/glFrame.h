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
    namespace gl4
    {
        //---

        // a frame in progress
        class Frame : public base::NoCopy
        {
            RTTI_DECLARE_POOL(POOL_API_RUNTIME)

        public:
            Frame(DeviceThread* thread);
            ~Frame(); // calls all the callbacks

            //--

            // attach a frame
            void attachPendingFrame();

            // attach a recorded frame
            void attachRecordedFrame(GLsync sync);

            // look to see if this sequence has ended
            // NOTE: will return true if all of the fences in the sequence are completed
            bool checkFences();

            // register a completion callback
            void registerCompletionCallback(FrameCompletionCallback callback);

            // register an object for deferred deletion
            void registerObjectForDeletion(Object* obj);

            //--

        private:
            uint32_t m_numDeclaredFrames;
            uint32_t m_numRecordedFrames;
            base::Array<GLsync> m_pendingFences;
            base::SpinLock m_pendingFencesLock;

            base::Array<FrameCompletionCallback*> m_completionCallbacks;
            base::SpinLock m_completionCallbacksLock;

            base::InplaceArray<Object*, 128> m_deferedDeletionObjects;
            base::SpinLock m_deferedDeletionObjectsLock;

            DeviceThread* m_thread;
        };

        //---

        /// execute the command command buffer by building a Vulkan command buffer
        /// preforms all the necessary synchronizations and data transfers, the generated command buffer is ready to use
        /// all the commands are executed on a single queue and fill a single command buffer
        extern void ExecuteCommands(Device* drv, DeviceThread* thread, Frame* seq, PerformanceStats* stats, command::CommandBuffer* masterCommandBuffer);

        //---

    } // gl4
} // rendering