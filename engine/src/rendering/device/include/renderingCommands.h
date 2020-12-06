/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#pragma once

#include "renderingObject.h"
#include "renderingDescriptorID.h"
#include "renderingFramebuffer.h"
#include "renderingResources.h"
#include "renderingDeviceApi.h"

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
        };

        RENDER_DECLARE_OPCODE_DATA(SwapOutput)
        {
            ObjectID output;
            uint8_t swap = 1;
        };

        //---
        // DYNAMIC STATE
        //---

		RENDER_DECLARE_OPCODE_DATA(SetLineWidth)
		{
			float width = 1.0f;
		};


        RENDER_DECLARE_OPCODE_DATA(SetBlendColor)
        {
            float color[4]; // NOTE: we avoid using aligned types in command buffer, hence the float[4] instead of Vector4
        };

        RENDER_DECLARE_OPCODE_DATA(SetDepthClip)
        {
			float min = 0.0f;
			float max = 0.0f;
        };

        RENDER_DECLARE_OPCODE_DATA(SetScissorRect)
        {
            base::Rect rect;
            uint8_t viewportIndex = 0;
        };

		RENDER_DECLARE_OPCODE_DATA(SetViewportRect)
		{
			base::Rect rect;
			float depthMin = 0.0f;
			float depthMax = 1.0f;
			uint8_t viewportIndex = 0;
		};

        RENDER_DECLARE_OPCODE_DATA(SetStencilReference)
        {
            uint8_t front = 0xFF;
            uint8_t back = 0xFF;
        };

        //---
        // FRAMEBUFFER
        //---

        RENDER_DECLARE_OPCODE_DATA(BeginPass)
        {
            FrameBuffer frameBuffer;
			ObjectID passLayoutId;
			uint8_t viewportCount = 1;
            bool hasResourceTransitions = false; // set if we have any layout transition while in this pass
        };

        RENDER_DECLARE_OPCODE_DATA(EndPass)
        {
            // TODO: resolve
        };

        RENDER_DECLARE_OPCODE_DATA(Resolve)
        {
            ObjectID source;
            ObjectID dest;
			uint8_t sourceMip = 0;
			uint8_t destMip = 0;
			uint16_t sourceSlice = 0;
			uint16_t destSlice = 0;
        };

        RENDER_DECLARE_OPCODE_DATA(ClearPassRenderTarget)
        {
            uint32_t index;
            float color[4];
        };

		RENDER_DECLARE_OPCODE_DATA(ClearRenderTarget)
		{
			ObjectID view;
			float color[4];
			uint32_t numRects = 0;
			// rects (base::Rect)
		};		

        RENDER_DECLARE_OPCODE_DATA(ClearPassDepthStencil)
        {
            uint8_t stencilValue = 0;
            uint8_t clearFlags = 0; // 1-depth, 2-stencil
            float depthValue = 1.0f;
        };

		RENDER_DECLARE_OPCODE_DATA(ClearDepthStencil)
		{
			ObjectID view;
			uint8_t stencilValue = 0;
			uint8_t clearFlags = 0; // 1-depth, 2-stencil
			float depthValue = 1.0f;
			uint32_t numRects = 0;
			// rects (base::Rect)
		};

        //---
        // PARAMETERS
        //---

        RENDER_DECLARE_OPCODE_DATA(BindDescriptor)
        {
            base::StringID binding;
            const DescriptorInfo* layout = nullptr;
            const DescriptorEntry* data = nullptr; // embedded
        };

        RENDER_DECLARE_OPCODE_DATA(UploadConstants)
        {
            uint32_t dataSize = 0;
            void* dataPtr = nullptr;

            mutable uint16_t bufferIndex = 0;
			mutable uint32_t bufferOffset = 0; // constant buffers are 64K ONLY
			OpUploadConstants* nextConstants = nullptr;
        };

        RENDER_DECLARE_OPCODE_DATA(UploadDescriptor)
        {
            uint32_t index = 0;
            const DescriptorInfo* layout = nullptr;
            OpUploadDescriptor* nextParameters = nullptr;
        };

        //---
        // GEOMETRY
        //---

        RENDER_DECLARE_OPCODE_DATA(BindVertexBuffer)
        {
            base::StringID bindpoint;
            uint32_t offset = 0;
            ObjectID id;
        };

        RENDER_DECLARE_OPCODE_DATA(BindIndexBuffer)
        {
            ImageFormat format = ImageFormat::UNKNOWN;
            uint32_t offset = 0;
            ObjectID id;
        };

        RENDER_DECLARE_OPCODE_DATA(Draw)
        {
            ObjectID pipelineObject;
            uint32_t firstVertex = 0;
            uint32_t vertexCount = 0;
            uint16_t firstInstance = 0;
            uint16_t numInstances = 0;
        };      

        RENDER_DECLARE_OPCODE_DATA(DrawIndexed)
        {
			ObjectID pipelineObject;
            uint32_t firstVertex = 0;
            uint32_t firstIndex = 0;
            uint32_t indexCount = 0;
            uint16_t firstInstance = 0;
            uint16_t numInstances = 0;
        };

        RENDER_DECLARE_OPCODE_DATA(Dispatch)
        {
			ObjectID pipelineObject;
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

		RENDER_DECLARE_OPCODE_DATA(ResourceLayoutBarrier)
		{
			ObjectID id;
			ResourceLayout sourceLayout = ResourceLayout::INVALID;
			ResourceLayout targetLayout = ResourceLayout::INVALID;
			uint8_t firstMip = 0;
			uint8_t numMips = 0;
			uint8_t firstSlice = 0;
			uint8_t numSlices = 0;
		};

		RENDER_DECLARE_OPCODE_DATA(UAVBarrier)
		{
			ObjectID viewId;
		};

		//---
		// RESOURCES
		//---

		RENDER_DECLARE_OPCODE_DATA(ClearBuffer)
		{
			ObjectID view; // writable view!
			ImageFormat clearFormat = ImageFormat::UNKNOWN;
			uint32_t numRects = 0;
			// ResourceClearRect[numRects]
			// payload contains clear data (up to 16 bytes)
		};

		RENDER_DECLARE_OPCODE_DATA(ClearImage)
		{
			ObjectID view; // writable view!
			ImageFormat clearFormat = ImageFormat::UNKNOWN;
			uint32_t numRects = 0;
			// ResourceClearRect[numRects]
			// payload contains clear data (up to 16 bytes)
		};


        RENDER_DECLARE_OPCODE_DATA(Copy)
        {
            ObjectID src;
            ObjectID dest;
			ResourceCopyRange srcRange;
			ResourceCopyRange destRange;
        };

        RENDER_DECLARE_OPCODE_DATA(Update)
        {
            ObjectID id; // resource (must be with the "dynamic" flag) - NOTE: it's NOT a view, it's directly the resource
			ResourceCopyRange range;
            void* dataBlockPtr = nullptr; // update data (byte-packed)
            uint32_t dataBlockSize = 0; // size of the update data

			OpUpdate* next = nullptr;

			mutable uint32_t stagingBufferOffset = 0; // assigned placement in the staging area
        };

        RENDER_DECLARE_OPCODE_DATA(Download)
        {
            ObjectID id;
			ObjectID areaId;
			uint32_t offsetInArea = 0;
			uint32_t sizeInArea = 0;
			IDownloadAreaObject* area = nullptr;
			IDownloadDataSink* sink = nullptr;
			ResourceCopyRange range;
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

		struct DescriptorTrackingEntry
		{
			const DescriptorEntry* dataPtr = nullptr;
			base::InplaceArray<DeviceObjectViewPtr, 10> views;
		};

		//--

    } // command
} // rendering