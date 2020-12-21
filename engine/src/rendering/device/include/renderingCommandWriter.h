/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#pragma once

#include "renderingDescriptorID.h"
#include "renderingResources.h"

namespace rendering
{
    struct FrameBufferViewportState;

    namespace command
    {
        struct OpBase;
        struct OpAllocTransientBuffer;
        struct OpBeginPass;
		struct OpUploadConstants;
		struct OpUpdate;

        class CommandBuffer;

        struct AcquiredOutput
        {
            uint32_t width = 0;
            uint32_t height = 0;
            RenderTargetViewPtr color; // NOTE: rt size may be bigger than window size
            RenderTargetViewPtr depth; // NOTE: may be null (some windows only have color surface)

            INLINE AcquiredOutput() {}
            INLINE operator bool() const { return width != 0 && height != 0; }
        };

		struct ResourceCurrentStateTrackingRecord : public base::NoCopy
		{
			bool tracksSubResources = false;
			uint8_t numImageMips = 0;
			uint16_t numImageSlices = 0;

			struct SubResourceState
			{
				ResourceLayout currentLayout;
				ResourceLayout firstKnownLayout;
			};

			uint32_t numSubResources = 0; // at least 1
			SubResourceState subResources[1];
		};

		struct StashedFrameBuffer
		{
			//FrameBuffer fb;
			uint8_t numViewports = 0;
			base::InplaceArray<base::Rect, 1> viewports;
			base::InplaceArray<base::Vector2, 1> depthRanges;
		};

        /// helper class for writing the micro operations stream
        /// NOTE: thread safe to write using multiple threads once inside a pass
        class RENDERING_DEVICE_API CommandWriter : public base::NoCopy
        {
        public:
            CommandWriter(std::nullptr_t); // unattached
            CommandWriter(CommandBuffer* buffer, base::StringView scopeName = base::StringView()); // NOTE: buffer is reset, that's the only legal way we can write to it
            CommandWriter(base::StringView scopeName = base::StringView()); // NOTE: buffer is reset, that's the only legal way we can write to it
            ~CommandWriter();

            //--

            // attach command buffer to empty writer
            void attachBuffer(CommandBuffer* buffer);

            // release buffer from the writer, finished it as well
            CommandBuffer* release(bool finishRecording = true);

            //--

            // acquire output (back buffer) render targets from render output
            // NOTE: the acquire may fail (if output is lost, if so, rendering should not continue)
            AcquiredOutput opAcquireOutput(IOutputObject* output);

            // swap the previously acquired back buffer
            void opSwapOutput(IOutputObject* output, bool doSwap=true);
            
            //--

            // bind a frame buffer for pixel rendering, only one frame buffer can be bound at any given time
            // NOTE: draw commands only work when there is bound pass
            // NOTE: starting a pass resets ALL of the active graphics render states (blending, depth, stencil, etc...)
			// NOTE: starting a pass resets all viewports and scissor states to full render target area unless specified differently in the frame buffer
			// NOTE: number of viewports must be known at the beginning of the pess
            void opBeingPass(const FrameBuffer& frameBuffer, uint8_t viewportCount = 1, const base::Rect& drawArea = base::Rect::EMPTY());

            // finish rendering to a frame buffer, optionally resolve MSSA targets into non-MSAA ones
            void opEndPass();

			// clear given frame buffer without the whole hassle of "being/end pass"
			void opClearFrameBuffer(const FrameBuffer& frameBuffer, const base::Rect* area = nullptr);

            /// fill current color render target to single color
            void opClearPassRenderTarget(uint32_t index, const base::Vector4& color);

            /// fill current depth/stencil render target to given depth value
            void opClearPassDepthStencil(float depth, uint8_t stencil, bool doClearDepth = true, bool doClearStencil = true);

            /// resolve a MSAA source into a non-msaa destination, average selected samples, for depth targets we can only select one sample
			/// NOTE: we are resolving resources, not views
            void opResolve(const ImageObject* msaaSource, const ImageObject* nonMsaaDest, uint8_t sourceMip = 0, uint8_t destMip = 0, uint16_t sourceSlice = 0, uint16_t destSlice = 0);

            //---

            /// begin a debug block
            void opBeginBlock(base::StringView name, base::StringView file="", uint32_t line=0);

            /// end debug block
            void opEndBlock();

            //--

            /// create and attach at this point a new command buffer, commands from that command buffer will be inserted here
            /// NOTE: we don't have to fill the command buffer right away and we may do it on a fiber but it must finish recording before we submit work to rendering device
            /// NOTE: the created command buffer inherits currently set descriptors
            CommandBuffer* opCreateChildCommandBuffer(bool inheritParameters=true);

            /// attach external command buffer at this point in the frame sequence
            /// NOTE: we don't have to fill the command buffer right away and we may do it on a fiber but it must finish recording before we submit work to rendering device
            /// NOTE: the created command buffer WILL NOT inherit anything, also, this command buffer cannot be attached while in pass
            void opAttachChildCommandBuffer(CommandBuffer* buffer);

            //--

			/// clear part of writable buffer
			void opClearWritableBuffer(const BufferWritableView* bufferView, const void* clearValue = nullptr, uint32_t offset = 0, uint32_t size = INDEX_MAX);

			/// clear regions in writable buffer
			void opClearWritableBufferRects(const BufferWritableView* bufferView, const void* clearValue = nullptr, const ResourceClearRect* rects = nullptr, uint32_t numRects = 0);

			//--

            /// clear whole view of writable image with custom value of with zeros
			/// NOTE: this requires writable view of the image (mostly to accommodate DX that requires UAV for clearing)
			void opClearWritableImage(const ImageWritableView* view, const void* clearValue = nullptr);

			/// clear parts in view of writable image with custom value of with zeros
			/// NOTE: this requires writable view of the image (mostly to accommodate DX that requires UAV for clearing)
			void opClearWritableImageRects(const ImageWritableView* view, const void* clearValue = nullptr, const ResourceClearRect* rects = nullptr, uint32_t numRects = 0);

			//--

            /// clear buffer with custom value or with zero if no value was provided
            void opClearRenderTarget(const RenderTargetView* view, const base::Vector4& values, const base::Rect* rects = nullptr, uint32_t numRects = 0);

            /// clear buffer with custom value or with zero if no value was provided
            void opClearDepthStencil(const RenderTargetView* view, bool doClearDepth, bool doClearStencil, float clearDepth, uint32_t clearStencil, const base::Rect* rects = nullptr, uint32_t numRects = 0);

            //---

            /// bind vertex buffer at specified slot
            void opBindVertexBuffer(base::StringID name, const BufferObject* buffer, uint32_t offset = 0);

            /// unbind vertex buffer from specified slot
            void opUnbindVertexBuffer(base::StringID name);

            //---

            /// bind index buffer
            void opBindIndexBuffer(const BufferObject* buffer, ImageFormat indexFormat = ImageFormat::R16_UINT, uint32_t offset = 0);

            /// unbind vertex buffer from specified slot
            void opUnbindIndexBuffer();

            //---

            /// change rendering viewport
            /// NOTE: viewport is always reset at the beginning of each pass
            void opSetViewportRect(uint8_t viewportIndex, const base::Rect& viewportRect, float depthMin = 0.0f, float depthMax = 1.0f);
            void opSetViewportRect(uint8_t viewportIndex, int x, int y, int w, int h, float depthMin = 0.0f, float depthMax = 1.0f);

            /// set blend constant
			void opSetBlendConstant(const base::Color& color);
			void opSetBlendConstant(const base::Vector4& color);
			void opSetBlendConstant(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f);

            /// set line with for line rendering
            void opSetLineWidth(float lineWidth);

            /// NOTE: scissor is always reset at the beginning of each pass
            void opSetScissorRect(uint8_t viewportIndex, const base::Rect& scissorRect);
            void opSetScissorRect(uint8_t viewportIndex, int x, int y, int w, int h);
            void opSetScissorBounds(uint8_t viewportIndex, int x0, int y0, int x1, int y1);

            /// set the stencil reference value
            void opSetStencilReferenceValue(uint8_t value);
            void opSetStencilReferenceValue(uint8_t frontValue, uint8_t backValue);

            /// set dynamic depth clip state and ranges
			/// NOTE: works only for states in which it's enabled in the StaticStates
            void opSetDepthClip(float minBounds, float maxBounds);
            
            //---
			// drawing

            /// draw non-indexed geometry
            void opDraw(const GraphicsPipelineObject* po, uint32_t firstVertex, uint32_t vertexCount);

            /// draw non-indexed geometry with instancing
            void opDrawInstanced(const GraphicsPipelineObject* po, uint32_t firstVertex, uint32_t vertexCount, uint16_t firstInstance, uint16_t numInstances);

            /// draw indexed geometry
            void opDrawIndexed(const GraphicsPipelineObject* po, uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount);

            /// draw indexed geometry
            void opDrawIndexedInstanced(const GraphicsPipelineObject* po, uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount, uint16_t firstInstance, uint16_t numInstances);

            /// dispatch compute shader GROUPS
            void opDispatchGroups(const ComputePipelineObject* po, uint32_t countX = 1, uint32_t countY = 1, uint32_t countZ = 1);

			/// dispatch compute shader, count is given in threads
			void opDispatchThreads(const ComputePipelineObject* po, uint32_t threadCountX = 1, uint32_t threadCountY = 1, uint32_t threadCountZ = 1);

			//---
			// simple indirect drawing

			// draw vertices indirectly (via data in gpu buffer)
			void opDrawIndirect(const GraphicsPipelineObject* po, const BufferObject* buffer, uint32_t offsetInBuffer);

			// draw indexed vertices indirectly (via data in gpu buffer)
			void opDrawIndexedIndirect(const GraphicsPipelineObject* po, const BufferObject* buffer, uint32_t offsetInBuffer);

			/// dispatch compute shader GROUPS indirectly
			void opDispatchGroupsIndirect(const ComputePipelineObject* po, const BufferObject* buffer, uint32_t offsetInBuffer);

            //---
			// inlined resource updates
			/// NOTE: inlined updates are not intended to update large portions of resources for that device->asyncCopy should be used

			// update part of dynamic resource, returns pointer to memory where the update can be written
			// NOTE: for images the memory assumes native image layout (packed pixels) and number of pixels equal to sizeX*sizeY*sizeZ
			void* opUpdateDynamicPtr(const IDeviceObject* dynamicObject, const ResourceCopyRange& range);

			/// update part of typed dynamic resource
			/// the source data can be copied into the command buffer or kept as a separate buffer
			template< typename T >
			INLINE T* opUpdateDynamicBufferPtrN(const IDeviceObject* dynamicObject, uint32_t first, uint32_t count)
			{
				ResourceCopyRange range;
				range.buffer.offset = first * sizeof(T);
				range.buffer.size = count * sizeof(T);
				return (T*)opUpdateDynamicPtr(dynamicObject, range);
			}

			/// update part of typed dynamic resource
			/// the source data can be copied into the command buffer or kept as a separate buffer
			template< typename T >
			INLINE T* opUpdateDynamicImagePtrN(const IDeviceObject* dynamicObject, uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint8_t mipIndex=0, uint32_t sliceIndex=0)
			{
				ResourceCopyRange range;
				range.image.firstMip = mipIndex;
				range.image.numMips = 1;
				range.image.firstSlice = sliceIndex;
				range.image.numSlices = 1;
				range.image.offsetX = x;
				range.image.offsetY = y;
				range.image.sizeX = width;
				range.image.sizeY = height;
				range.image.sizeZ = 1;
				return (T*)opUpdateDynamicPtr(dynamicObject, range);
			}
			
			//--

            /// inlined (executed between other calls) update part of dynamic image
            /// the source data is provided in form of an image view
            void opUpdateDynamicImage(const ImageObject* dynamicImage, const base::image::ImageView& updateData, uint8_t mipIndex = 0, uint32_t sliceIndex = 0, uint32_t offsetX=0, uint32_t offsetY=0, uint32_t offsetZ=0);

            /// update part of dynamic buffer
            /// the source data can be copied into the command buffer or kept as a separate buffer
            void opUpdateDynamicBuffer(const BufferObject* dynamicBuffer, uint32_t dataOffset, uint32_t dataSize, const void* dataPtr);

			//--

			/// copy content of two resources
			void opCopy(const IDeviceObject* src, const ResourceCopyRange& srcRange, const IDeviceObject* dest, const ResourceCopyRange& destRange);

			/// copy data between buffers
			void opCopyBuffer(const BufferObject* src, uint32_t srcOffset, const BufferObject* dest, uint32_t destOffset, uint32_t size);

			/// copy data between buffer and an image
			void opCopyBufferToImage(const BufferObject* src, uint32_t srcOffset, const ImageObject* dest, uint32_t offsetX, uint32_t offsetY, uint32_t offsetZ, uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint8_t mipIndex = 0, uint32_t sliceIndex = 0);

			/// copy data between image and buffer
			void opCopyImageToBuffer(const ImageObject* src, uint32_t offsetX, uint32_t offsetY, uint32_t offsetZ, uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, const BufferObject* dest, uint32_t destOffset, uint8_t mipIndex = 0, uint32_t sliceIndex = 0);

			//--

			/// finish UAV writes to resource so new reads can see them
			/// NOTE: no layout transition, expected resource layout is ResourceLayout::UAV
			void opTransitionFlushUAV(const BufferWritableView* bufferView);

			/// finish UAV writes to resource so new reads can see them
			/// NOTE: no layout transition, expected resource layout is ResourceLayout::UAV
			void opTransitionFlushUAV(const ImageWritableView* imageView);

            /// transition layout of whole resource (image or buffer)
            void opTransitionLayout(const IDeviceObject* image, ResourceLayout incomingLayout, ResourceLayout outgoingLayout);

            /// transition layout of an image resource (whole image)
            void opTransitionImageRangeLayout(const ImageObject* image, uint8_t firstMip, uint8_t numMips, ResourceLayout incomingLayout, ResourceLayout outgoingLayout);

            /// transition layout of an image resource (whole image)
            void opTransitionImageArrayRangeLayout(const ImageObject* image, uint8_t firstMip, uint8_t numMips, uint32_t firstSlice, uint32_t numSlices, ResourceLayout incomingLayout, ResourceLayout outgoingLayout);

			/// transition layout of an image resource (whole image)
			void opTransitionImageArrayRangeLayout(const ImageObject* image, uint32_t firstSlice, uint32_t numSlices, ResourceLayout incomingLayout, ResourceLayout outgoingLayout);

            //---

            /// bind new descriptor values to given slot, returns local (in command buffer) descriptor index
            void opBindDescriptorEntries(base::StringID bindingID, const DescriptorEntry* entries, uint32_t count);

			/// bind new descriptor entries specified in a table
			template< typename T >
			INLINE void opBindDescriptor(base::StringID bindingID, const T& data)
			{
				static_assert(!std::is_pointer<T>::value, "No pointers allowed here, use opBindDescriptorEntries");
				return opBindDescriptorEntries(bindingID, data, ARRAY_COUNT(data));
			}

            //---

            /// download content of a resource (or part of it), downloaded data is forwarded to sink (usually 2 frames later)
			void opDownloadData(const IDeviceObject* obj, const ResourceCopyRange& range, IDownloadAreaObject* area, uint32_t areaOffset, IDownloadDataSink* sink);

            //---

            /// trigger capture HACK for render doc
            void opTriggerCapture();

        private:
            void ensureMemory(uint32_t size);

            INLINE void* allocMemory(uint32_t size)
            {
                ensureMemory(size);

                auto* ret = m_writePtr;
                m_writePtr += size;
                return ret;
            }

            template< typename T >
            T* allocCommand(uint32_t extraSize = 0)
            {
                ensureMemory(sizeof(T) + extraSize);
                return T::Alloc<T>(m_writePtr, extraSize, m_lastCommand);
            }

            //---

            OpBase* m_lastCommand = nullptr;

            uint8_t* m_writePtr = nullptr;
            uint8_t* m_writeEndPtr = nullptr;
            CommandBuffer* m_writeBuffer = nullptr;

            OpBeginPass* m_currentPass = nullptr;
            uint8_t m_numOpenedBlocks = 0;
            uint8_t m_currentPassRts = 0;
			uint8_t m_currentPassViewports = 0;

#ifdef VALIDATE_VERTEX_LAYOUTS
            uint32_t m_currentIndexBufferElementCount = 0;
			BufferObjectPtr m_currentIndexBuffer;

            base::HashMap<base::StringID, uint32_t> m_currentVertexBufferRemainingSize;
			base::HashMap<base::StringID, BufferObjectPtr> m_currentVertexBuffers;
#endif

			base::HashMap<base::StringID, DescriptorID> m_currentParameterBindings;

#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES			
			base::HashMap<base::StringID, const DescriptorEntry*> m_currentParameterData;
			base::HashSet<DeviceObjectViewPtr> m_trackedObjectViews;
#endif

            bool m_isChildBufferWithParentPass = false;

			//--

#ifdef VALIDATE_RESOURCE_LAYOUTS
			mutable base::HashMap<ObjectID, ResourceCurrentStateTrackingRecord*> m_currentResourceState; 

			struct SubImageRegion
			{
				uint8_t firstMip = 0;
				uint8_t numMips = 0;
				uint16_t firstSlice = 0;
				uint16_t numSlices = 0;
			};

			bool ensureResourceState(const IDeviceObject* object, ResourceLayout layout, const SubImageRegion* subImageRegion = nullptr, ResourceLayout newState = ResourceLayout::INVALID);
			ResourceCurrentStateTrackingRecord* createResourceStateTrackingEntry(const IDeviceObject* obj);
#endif

			//--

            void detachBuffer(bool finishRecording);

			bool validateParameterBindings(const ShaderMetadata* meta);
            bool validateDrawVertexLayout(const ShaderMetadata* meta, uint32_t requiredVertexCount, uint32_t requiredInstanceCount);
			bool validateDrawIndexLayout(uint32_t requiredElementCount);
			bool validateIndirectDraw(const BufferObject* buffer, uint32_t offsetInBuffer, uint32_t commandStride);
			bool validateFrameBuffer(const FrameBuffer& fb, uint32_t* outWidth=nullptr, uint32_t* outHeight = nullptr);

			DescriptorEntry* uploadDescriptor(DescriptorID layoutID, const DescriptorInfo* layout, const DescriptorEntry* entries, uint32_t count);
            void* allocConstants(uint32_t size, const command::OpUploadConstants*& outCommand);

			void linkUpdate(OpUpdate* op);
        };

        //--

        // scoped command block
        struct CommandWriterBlock : public base::NoCopy
        {
        public:
            INLINE CommandWriterBlock(CommandWriter& cmd, base::StringView name)
            {
                if (name)
                {
                    m_writer = &cmd;
                    cmd.opBeginBlock(name);
                }
            }

            INLINE ~CommandWriterBlock()
            {
                if (m_writer)
                    m_writer->opEndBlock();
            }

        private:
            CommandWriter* m_writer = nullptr;
        };

        //--

    } // command
} // rendering