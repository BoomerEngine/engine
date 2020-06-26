/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#pragma once

#include "renderingObject.h"
#include "renderingImageView.h"
#include "renderingBufferView.h"
#include "renderingConstantsView.h"
#include "renderingParametersView.h"
#include "renderingParametersLayoutID.h"
#include "renderingFramebuffer.h"

#include "base/image/include/imageVIew.h"

namespace rendering
{
    namespace command
    {
        /// rendering communicates with the outside word with the stream of micro operations
        /// they are kind of like a command buffer but with the advantage that we can generate the offline and the backend tools can inspect them
        /// this + the fragments and the links between them is crucial idea behind the ability of the backend compiler to generate all the possible pipeline states
        enum class CommandCode : uint16_t
        {
#define RENDER_COMMAND_OPCODE(x) x,
#include "renderingCommandOpcodes.inl"
#undef RENDER_COMMAND_OPCODE
        };

        //--

#pragma pack(push)
#pragma pack(4) // must be exactly 4!

        // NOTE: the destructor of the Op structures are NEVER CALLED, if you store something there that requires destruction you will leak memory
        // Don't worry, you will learn about it soon, really soon
        struct OpBase
        {
            static const uint32_t MAGIC = 0xFF00BB00;

#ifndef BUILD_RELEASE
            uint32_t magic = 0;
#endif
            
            CommandCode op = CommandCode::Nop;
            uint16_t offsetToNext = 0; // offset to next from current

            INLINE OpBase* setup(CommandCode op_)
            {
                op = op_;
                offsetToNext = 0;
#ifndef BUILD_RELEASE
                magic = MAGIC;
#endif
                return this;
            }

            INLINE OpBase* setup(CommandCode op_, OpBase* prev)
            {
                op = op_;
                offsetToNext = 0;
#ifndef BUILD_RELEASE
                magic = MAGIC;
#endif
                auto offset = (uint8_t*)this - (uint8_t*)prev;
                DEBUG_CHECK(offset >= 0 && offset < UINT16_MAX);
                prev->offsetToNext = offset;

                return this;                
            }

            template< typename T >
            static INLINE T* Alloc(uint8_t*& ptr, uint32_t extraSize, OpBase*& lastCommand)
            {
                auto* cmd = (T*)ptr;
                ptr += sizeof(T) + extraSize;
                lastCommand = cmd->setup(T::OP, lastCommand);
                return cmd;
            }

            template< typename T >
            static INLINE T* Alloc(uint8_t*& ptr)
            {
                auto* cmd = (T*)ptr;
                ptr += sizeof(T);
                cmd->setup(T::OP);
                return cmd;
            }

            INLINE const OpBase* next() const
            {
                return offsetToNext ? base::OffsetPtr(this, offsetToNext) : nullptr;
            }
        };

#ifdef BUILD_RELEASE
        static_assert(sizeof(OpBase) == 4, "Please don't make me bigger");
#else
        static_assert(sizeof(OpBase) == 8, "Please don't make me bigger");
#endif

        template< CommandCode opcode, typename SelfT >
        struct OpBaseT : public OpBase
        {
            static const auto OP = opcode;

            // pointer to the command payload, usually just after the command
            template< typename T = uint8_t >
            INLINE T* payload() { return (T*)((const char*)this + sizeof(SelfT)); }

            template< typename T = uint8_t >
            INLINE const T* payload() const { return (const T*)((const char*)this + sizeof(SelfT)); }
        };

#define RENDER_DECLARE_OPCODE_DATA(op_) struct Op##op_ : public OpBaseT<CommandCode::op_, Op##op_>

        RENDER_DECLARE_OPCODE_DATA(Nop)
        {
        };

        RENDER_DECLARE_OPCODE_DATA(Hello)
        {
        };

        RENDER_DECLARE_OPCODE_DATA(NewBuffer)
        {
            OpBase* firstInNewBuffer = nullptr;
        };

        RENDER_DECLARE_OPCODE_DATA(ChildBuffer)
        {
            CommandBuffer* childBuffer = nullptr;
            OpChildBuffer* nextChildBuffer = nullptr;
            bool insidePass = false; // command buffer was attached inside a pass
            bool inheritsParameters = false; // command buffer inherits parameters
        };

        RENDER_DECLARE_OPCODE_DATA(AcquireOutput)
        {
            ObjectID output;
            ImageView colorView;
            ImageView depthView;
            base::Rect viewport;
        };

        RENDER_DECLARE_OPCODE_DATA(SwapOutput)
        {
            ObjectID output;
            uint8_t swap = 1;
        };

        //---
        // BLEND
        //---

        RENDER_DECLARE_OPCODE_DATA(SetColorMask)
        {
            uint8_t rtIndex = 0;
            uint8_t colorMask = 0xF;
        };

        RENDER_DECLARE_OPCODE_DATA(SetBlendState)
        {
            BlendState state;
            uint8_t rtIndex = 0;
        };

        RENDER_DECLARE_OPCODE_DATA(SetBlendColor)
        {
            float color[4]; // NOTE: we avoid using aligned types in command buffer, hence the float[4] instead of Vector4
        };

        //---
        // DEPTH
        //---

        RENDER_DECLARE_OPCODE_DATA(SetDepthState)
        {
            DepthState state;
        };

        RENDER_DECLARE_OPCODE_DATA(SetDepthBiasState)
        {
            DepthBiasState state;
        };

        RENDER_DECLARE_OPCODE_DATA(SetDepthClipState)
        {
            DepthClipState state;
        };

        //---
        // STENCIL
        //---

        RENDER_DECLARE_OPCODE_DATA(SetScissorRect)
        {
            base::Rect rect;
            uint8_t viewportIndex = 0;
        };

        RENDER_DECLARE_OPCODE_DATA(SetScissorState)
        {
            bool state = false;
        };

        RENDER_DECLARE_OPCODE_DATA(SetStencilState)
        {
            StencilState state;
        };

        RENDER_DECLARE_OPCODE_DATA(SetStencilWriteMask)
        {
            uint8_t front = 0xFF;
            uint8_t back = 0xFF;
        };

        RENDER_DECLARE_OPCODE_DATA(SetStencilCompareMask)
        {
            uint8_t front = 0xFF;
            uint8_t back = 0xFF;
        };

        RENDER_DECLARE_OPCODE_DATA(SetStencilReference)
        {
            uint8_t front = 0xFF;
            uint8_t back = 0xFF;
        };


        //---
        // TINY STATES
        //---

        RENDER_DECLARE_OPCODE_DATA(SetCullState)
        {
            CullState state;
        };

        RENDER_DECLARE_OPCODE_DATA(SetFillState)
        {
            FillState state;
        };

        RENDER_DECLARE_OPCODE_DATA(SetPrimitiveAssemblyState)
        {
            PrimitiveAssemblyState state;
        };

        RENDER_DECLARE_OPCODE_DATA(SetMultisampleState)
        {
            MultisampleState state;
        };

        //---
        // FRAMEBUFFER
        //---

        RENDER_DECLARE_OPCODE_DATA(BeginPass)
        {
            FrameBuffer frameBuffer;
            uint8_t numViewports = 1;
            bool hasBarriers = false; // TODO: track dependencies better!
            bool hasInitialViewportSetup = false;
        };

        RENDER_DECLARE_OPCODE_DATA(EndPass)
        {
            // TODO: resolve
        };

        RENDER_DECLARE_OPCODE_DATA(Resolve)
        {
            ImageView msaaSource;
            ImageView nonMsaaDest;
            uint8_t sampleMask = 0xFF;
            uint8_t depthSampleIndex = 0;
        };

        RENDER_DECLARE_OPCODE_DATA(ClearPassColor)
        {
            uint32_t index;
            float color[4];
        };

        RENDER_DECLARE_OPCODE_DATA(ClearPassDepthStencil)
        {
            uint8_t stencilValue = 0;
            uint8_t clearFlags = 0; // 1-depth, 2-stencil
            float depthValue = 1.0f;
        };

        RENDER_DECLARE_OPCODE_DATA(SetViewportRect)
        {
            base::Rect rect;
            uint8_t viewportIndex = 0;
        };

        RENDER_DECLARE_OPCODE_DATA(SetViewportDepthRange)
        {
            float minZ = 0.0f;
            float maxZ = 1.0f;
            uint8_t viewportIndex = 0;
        };

        //---
        // PARAMETERS
        //---

        RENDER_DECLARE_OPCODE_DATA(BindParameters)
        {
            base::StringID binding;
            ParametersView view;
        };

        RENDER_DECLARE_OPCODE_DATA(UploadConstants)
        {
            uint32_t offset = 0;
            uint32_t dataSize = 0;
            void* dataPtr = nullptr;
            mutable uint32_t mergedRuntimeOffset = 0;
            OpUploadConstants* nextConstants = nullptr;
        };

        RENDER_DECLARE_OPCODE_DATA(UploadParameters)
        {
            uint32_t index = 0;
            const ParametersLayoutInfo* layout = nullptr;
            OpUploadParameters* nextParameters = nullptr;
        };

        //---
        // RESOURCES
        //---

        RENDER_DECLARE_OPCODE_DATA(ClearBuffer)
        {
            BufferView view;
        };

        RENDER_DECLARE_OPCODE_DATA(ClearImage)
        {
            ImageView view;
        };

        //---
        // GEOMETRY
        //---

        RENDER_DECLARE_OPCODE_DATA(BindVertexBuffer)
        {
            base::StringID bindpoint;
            uint32_t offset = 0;
            BufferView buffer;
        };

        RENDER_DECLARE_OPCODE_DATA(BindIndexBuffer)
        {
            ImageFormat format;
            uint32_t offset = 0;
            BufferView buffer;
        };

        RENDER_DECLARE_OPCODE_DATA(AllocTransientBuffer)
        {
            TransientBufferView buffer;
            OpAllocTransientBuffer* next = nullptr;
            uint32_t initializationDataSize = 0;
            void* initializationData = nullptr;
        };

        RENDER_DECLARE_OPCODE_DATA(Draw)
        {
            ObjectID shaderLibrary;
            PipelineIndex shaderIndex;
            uint32_t firstVertex;
            uint32_t vertexCount;
            uint16_t firstInstance;
            uint16_t numInstances;
        };      

        RENDER_DECLARE_OPCODE_DATA(DrawIndexed)
        {
            ObjectID shaderLibrary;
            PipelineIndex shaderIndex;
            uint32_t firstVertex;
            uint32_t firstIndex;
            uint32_t indexCount;
            uint16_t firstInstance;
            uint16_t numInstances;
        };

        RENDER_DECLARE_OPCODE_DATA(Dispatch)
        {
            ObjectID shaderLibrary;
            PipelineIndex shaderIndex;
            uint32_t counts[3];
        };

        //---
        // OTHERS
        //---

        RENDER_DECLARE_OPCODE_DATA(TriggerCapture)
        {
            // TODO: capture file path ?
        };

        RENDER_DECLARE_OPCODE_DATA(BeginBlock)
        {
        };

        RENDER_DECLARE_OPCODE_DATA(EndBlock)
        {
        };

        RENDER_DECLARE_OPCODE_DATA(ImageLayoutBarrier)
        {
            ImageView view;
            ImageLayout targetLayout;
        };

        RENDER_DECLARE_OPCODE_DATA(GraphicsBarrier)
        {
            Stage from;
            Stage to;
        };

        RENDER_DECLARE_OPCODE_DATA(UpdateDynamicImage)
        {
            ImageView view; // the image to update (must be with the "dynamic" flag)
            uint32_t placementOffset[3]; // update placement
            base::image::ImageView data; // image data, placed locally
            void* dataBlockPtr; // update data (byte-packed)
            uint32_t dataBlockSize; // size of the update data
        };

        RENDER_DECLARE_OPCODE_DATA(UpdateDynamicBuffer)
        {
            BufferView view; // the image to update (must be with the "dynamic" flag)
            uint32_t offset; // offset in the view
            uint32_t dataBlockSize; // size of the update data
            void* dataBlockPtr; // update data (byte-packed)
            OpUpdateDynamicBuffer* next;
            uint32_t stagingBufferOffset;
        };

        RENDER_DECLARE_OPCODE_DATA(SignalCounter)
        {
            base::fibers::WaitCounter counter;
            uint32_t valueToSignal;
        };

        RENDER_DECLARE_OPCODE_DATA(WaitForCounter)
        {
            base::fibers::WaitCounter counter;
        };

        RENDER_DECLARE_OPCODE_DATA(DownloadBuffer)
        {
            BufferView buffer;
            DownloadBuffer* ptr;
            uint32_t downloadOffset;
            uint32_t downloadSize;
        };

        RENDER_DECLARE_OPCODE_DATA(DownloadImage)
        {
            ImageView image;
            DownloadImage* ptr;
        };

#pragma pack(pop)
#undef RENDER_DECLARE_OPCODE_DATA

        //--

        // get next command from command stream
        INLINE const OpBase* GetNextCommand(const OpBase* op)
        {
#ifndef BUILD_RELEASE
            ASSERT_EX(op->magic == OpBase::MAGIC, "Invalid opcode magic - buffer overrun");
#endif
            const auto* next = op->next();
            while (next && next->op == CommandCode::NewBuffer)
                next = static_cast<const OpNewBuffer*>(next)->firstInNewBuffer;
            return next;
        }

        //--

    } // command
} // rendering