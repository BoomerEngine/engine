/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#pragma once

#include "renderingCommands.h"

namespace rendering
{
    namespace command
    {
        /// for a linked list of parameters to upload
        struct UploadParametersLinkEntry
        {
            uint32_t nextIndex = 0;
            OpUploadParameters* head = nullptr;
            OpUploadParameters* tail = nullptr;
            UploadParametersLinkEntry* next = nullptr;
        };

        /// various state data gathered from commands as we are recording
        struct CommandBufferGatheredState
        {
            // linked list of constant upload commands as well as total size of the upload memory for constants
            uint32_t totalConstantsUploadSize = 0;
            OpUploadConstants* constantUploadHead = nullptr;
            OpUploadConstants* constantUploadTail = nullptr;

            // linked list of transient buffer allocations
            OpAllocTransientBuffer* triansienBufferAllocHead = nullptr;
            OpAllocTransientBuffer* triansienBufferAllocTail = nullptr;

            // linked list of buffer updates
            OpUpdateDynamicBuffer* dynamicBufferUpdatesHead = nullptr;
            OpUpdateDynamicBuffer* dynamicBufferUpdatesTail = nullptr;

            // linked list of parameter uploads organized by the type
            static const uint32_t MAX_LAYOUTS = 256;
            UploadParametersLinkEntry* parameterUploadActiveList = nullptr;
            UploadParametersLinkEntry parameterUploadEntries[MAX_LAYOUTS];
        };

        /// Buffer with all the commands for rendering
        /// NOTE: Command buffers are intended to be submitted the frame they are recorded since the recorded resource IDs are only guaranteed to be valid till a given frame ends
        /// NOTE: Command buffers are single threaded beasts, only one thread with a writer can be attached to a command buffer at given time
        /// NOTE: technically unfinished command buffer can be submitted if we insert a fence there (WaitForSignal) to wait 
        class RENDERING_DRIVER_API CommandBuffer : public base::NoCopy, public base::mem::GlobalPoolObject<POOL_RENDERING_COMMAND_BUFFER>
        {
        public:
            //--

            // allocate a new buffer, call "release" when no longer needed
            static CommandBuffer* Alloc();

            //--

            // finish recording on this buffer
            void finishRecording();

            // release buffer to pool, called once device finishes converting this command buffer into native one
            void release();

            //--

            // is the buffer "opened", ie. still being written to ?
            INLINE bool opened() const { return m_writingThread != 0; }

            // get list of recorded commands (use GetNextCommand to navigate)
            INLINE const OpBase* commands() const { return m_commands; }

            // get gathered state that can be used to speed up recording of native command buffers
            INLINE const CommandBufferGatheredState& gatheredState() const { return m_gatheredState; }

            // did the recording finish ?
            INLINE bool closed() const { return m_finished.load() != 0; }

            //--

            // visit child buffers in order
            bool enumChildren(const std::function<bool(CommandBuffer * buffer)>& enumFunc);

            // visit the whole hierarchy in order
            bool visitHierarchy(const std::function<bool(CommandBuffer * buffer)>& enumFunc);

        private:
            CommandBuffer();
            ~CommandBuffer();

            //--

            base::mem::PageCollection* m_pages;

            //--

            const OpBase* m_commands = nullptr;
            OpBase* m_lastCommand = nullptr;
            OpNewBuffer* m_lastBufferTail = nullptr;
            OpChildBuffer* m_firstChildBuffer = nullptr;
            OpChildBuffer* m_lastChildBuffer = nullptr;

            uint8_t* m_currentWritePtr = nullptr;
            uint8_t* m_currentWriteEndPtr = nullptr;

            std::atomic<uint32_t> m_finished = 0;

            base::InplaceArray<DownloadBufferPtr, 4> m_downloadBuffers;
            base::InplaceArray<DownloadImagePtr, 4> m_downloadImages;

            base::HashMap<base::StringID, ParametersLayoutID> m_activeParameterBindings;

            bool m_isChildCommandBuffer = false;
            OpBeginPass* m_parentBufferBeginPass = nullptr;

            volatile uint32_t m_writingThread = 0; // non zero if we are opened for writing

            //--

            CommandBufferGatheredState m_gatheredState;

            //---

            void beginWriting();
            void endWriting();

            //--

            friend class CommandWriter;
        };

        //--

        // validate command buffer
        extern RENDERING_DRIVER_API void ValidateCommandBuffer(CommandBuffer* buffer);

        //--

    } // command
} // rendering