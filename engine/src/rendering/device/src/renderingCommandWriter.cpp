/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#include "build.h"
#include "renderingCommands.h"
#include "renderingCommandBuffer.h"
#include "renderingCommandWriter.h"
#include "renderingFramebuffer.h"
#include "renderingDescriptorID.h"
#include "renderingDescriptorInfo.h"
#include "renderingObject.h"
#include "renderingOutput.h"
#include "renderingImage.h"
#include "renderingBuffer.h"
#include "renderingShader.h"
#include "renderingDeviceService.h"
#include "renderingDeviceApi.h"
#include "renderingDescriptor.h"
#include "renderingPipeline.h"
#include "renderingShaderMetadata.h"

#include "base/containers/include/bitUtils.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"
#include "base/system/include/thread.h"
#include "base/memory/include/pageCollection.h"
#include "base/image/include/imageRect.h"

namespace rendering
{
    namespace command
    {

		//--

		static uint32_t CalcUpdateMemorySize(ImageFormat format, const ResourceCopyRange& range)
		{
			const auto& formatInfo = GetImageFormatInfo(format);
			if (formatInfo.compressed)
			{
				DEBUG_CHECK_RETURN_V((range.image.offsetX & 3) == 0, 0);
				DEBUG_CHECK_RETURN_V((range.image.offsetY & 3) == 0, 0);
				DEBUG_CHECK_RETURN_V((range.image.offsetZ & 3) == 0, 0);
				DEBUG_CHECK_RETURN_V(range.image.sizeZ == 1, 0);

				const auto alignedSizeX = base::Align<uint32_t>(range.image.sizeX, 4);
				const auto alignedSizeY = base::Align<uint32_t>(range.image.sizeY, 4);
				return alignedSizeX * alignedSizeY * formatInfo.bitsPerPixel / 8;
			}
			else
			{
				return range.image.sizeX * range.image.sizeY * range.image.sizeZ * formatInfo.bitsPerPixel / 8;
			}
		}

        //--

        CommandWriter::CommandWriter(std::nullptr_t)
        {
        }

        CommandWriter::CommandWriter(CommandBuffer* buffer, base::StringView scopeName /*= base::StringView()*/)
        {
            attachBuffer(buffer);
            
            if (!scopeName.empty())
                opBeginBlock(scopeName);
        }

        CommandWriter::CommandWriter(base::StringView scopeName /*= base::StringView()*/)
        {
            attachBuffer(CommandBuffer::Alloc());

            if (!scopeName.empty())
                opBeginBlock(scopeName);
        }

        CommandWriter::~CommandWriter()
        {
            if (m_writeBuffer)
                detachBuffer(true);
        }

        void CommandWriter::attachBuffer(CommandBuffer* buffer)
        {
            DEBUG_CHECK_EX(m_writeBuffer == nullptr, "Already attached to buffer");
            DEBUG_CHECK_EX(m_currentPass == nullptr, "We should not be insdie a pass");

            // bind
            m_writeBuffer = buffer;
            buffer->beginWriting();

            // get carried over parameter bindings
			m_currentParameterBindings = std::move(buffer->m_activeParameterBindings);

#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES
			m_currentParameterData = std::move(buffer->m_activeParameterData);
#endif

            m_currentPass = buffer->m_parentBufferBeginPass;
            m_isChildBufferWithParentPass = (buffer->m_parentBufferBeginPass != nullptr);

            // copy command buffer state into writer since writer is usually on stack 
            m_lastCommand = m_writeBuffer->m_lastCommand;
            m_writePtr = m_writeBuffer->m_currentWritePtr;
            m_writeEndPtr = m_writeBuffer->m_currentWriteEndPtr - sizeof(OpNewBuffer);

            // we should have the guard at the memory
            DEBUG_CHECK_EX(((OpBase*)m_writePtr)->op == CommandCode::NewBuffer, "Trying to write to improperly maintained command buffer");
#ifndef PLATFORM_RELEASE
            DEBUG_CHECK_EX(((OpBase*)m_writePtr)->magic == OpBase::MAGIC, "Trying to write to improperly maintained command buffer");
#endif
        }

        void CommandWriter::detachBuffer(bool finishRecording)
        {
            DEBUG_CHECK_EX(m_writePtr, "Nothing to detach");
            DEBUG_CHECK_EX(m_currentPass == nullptr || m_isChildBufferWithParentPass, "We are still in a pass");

            if (m_writeBuffer)
            {
                while (m_numOpenedBlocks > 0)
                    opEndBlock();

                auto* endBuffer = (OpNewBuffer*)m_writePtr;
                endBuffer->setup(CommandCode::NewBuffer);
                endBuffer->firstInNewBuffer = nullptr;

                m_writeBuffer->m_currentWritePtr = m_writePtr;
                m_writeBuffer->m_currentWriteEndPtr = m_writeEndPtr;
                m_writeBuffer->m_lastCommand = m_lastCommand;

                m_writeBuffer->endWriting();

                if (finishRecording)
                {
                    m_writeBuffer->finishRecording();

                    m_currentParameterBindings.reset();

#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES
					m_currentParameterData.reset();					
#endif
                }
                else
                {
                    m_writeBuffer->m_activeParameterBindings = std::move(m_currentParameterBindings);

#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES
					m_writeBuffer->m_activeParameterData = std::move(m_currentParameterData);
#endif
                }

                m_writePtr = nullptr;
                m_writeEndPtr = nullptr;
                m_writeBuffer = nullptr;
                m_lastCommand = nullptr;

                m_currentPass = nullptr;
                m_numOpenedBlocks = 0;
                m_currentPassRts = 0;
                //m_currentPassViewports = 0;

#ifdef VALIDATE_VERTEX_LAYOUTS
                m_currentIndexBufferElementCount = 0;
                m_currentVertexBufferRemainingSize.reset();
				m_currentVertexBuffers.reset();
#endif
            }
        }

        CommandBuffer* CommandWriter::release(bool finishRecording /*= true*/)
        {
            CommandBuffer* ret = nullptr;

            if (m_writeBuffer)
            {
                ret = m_writeBuffer;
                detachBuffer(finishRecording);
            }

            return ret;
        }

        void CommandWriter::ensureMemory(uint32_t size)
        {
            if (m_writePtr + size <= m_writeEndPtr)
                return;

            // finish current buffer
            auto* newBuffer = OpBase::Alloc<OpNewBuffer>(m_writePtr, 0, m_lastCommand);

            // allocate new buffer
            m_writePtr = (uint8_t*)m_writeBuffer->m_pages->allocatePage();
            m_writeEndPtr = m_writePtr + m_writeBuffer->m_pages->pageSize() - sizeof(OpNewBuffer); // we ALWAYS must be able to write the OpNewBuffer

            // write a dummy command to mark start of new buffer
            m_lastCommand = OpBase::Alloc<OpNop>(m_writePtr);
            newBuffer->firstInNewBuffer = m_lastCommand;
        }

        AcquiredOutput CommandWriter::opAcquireOutput(IOutputObject* output)
        {
            DEBUG_CHECK_RETURN_V(output, AcquiredOutput());

            AcquiredOutput ret;
            base::Point size;
            if (output->prepare(&ret.color, &ret.depth, size))
            {
                ret.width = size.x;
                ret.height = size.y;
				ret.layout = output->layout();

                auto op = allocCommand<OpAcquireOutput>();
                op->output = output->id();
            }

            return ret;
        }

        void CommandWriter::opSwapOutput(IOutputObject* output, bool doSwap /*= true*/)
        {
            DEBUG_CHECK_RETURN(output);

            auto op = allocCommand<OpSwapOutput>();
            op->output = output->id();
            op->swap = doSwap ? 1 : 0;
        }

        void CommandWriter::opTriggerCapture()
        {
            auto op  = allocCommand<OpTriggerCapture>();
        }

		bool CommandWriter::validateFrameBuffer(const FrameBuffer& frameBuffer, const GraphicsPassLayoutObject* layout, uint32_t* outWidth /*= nullptr*/, uint32_t* outHeight /*= nullptr*/)
		{
			auto frameBufferValid = frameBuffer.validate(outWidth, outHeight);
			DEBUG_CHECK_RETURN_EX_V(frameBufferValid, "Cannot enter a pass with invalid frame buffer", false);

			for (uint32_t i = 0; i < FrameBuffer::MAX_COLOR_TARGETS; ++i)
			{
				if (frameBuffer.color[i].viewPtr)
				{
					const auto format = frameBuffer.color[i].viewPtr->format();
					const auto expectedFormat = layout->layout().color[i].format;
					if (expectedFormat != ImageFormat::UNKNOWN)
					{
						DEBUG_CHECK_RETURN_EX_V(expectedFormat == format, base::TempString("Color target {} in frame buffer has layout {} but {} is expected by pass layout", i, format, expectedFormat), false);
					}
					else
					{
						DEBUG_CHECK_RETURN_EX_V(ImageFormat::UNKNOWN == format, base::TempString("Color target {} in frame buffer is bound with format '{}' but pass layout expects it's not", i, format), false);
					}
				}
				else
				{
					DEBUG_CHECK_RETURN_EX_V(layout->layout().color[i].format == ImageFormat::UNKNOWN, base::TempString("Color target {} in frame buffer is not specified but it's expected by pass layout", i), false);
				}
			}

			if (frameBuffer.depth.viewPtr)
			{
				const auto format = frameBuffer.depth.viewPtr->format();
				const auto expectedFormat = layout->layout().depth.format;
				if (expectedFormat != ImageFormat::UNKNOWN)
				{
					DEBUG_CHECK_RETURN_EX_V(expectedFormat == format, base::TempString("Depth target in frame buffer has layout {} but {} is expected by pass layout", format, expectedFormat), false);
				}
				else
				{
					DEBUG_CHECK_RETURN_EX_V(ImageFormat::UNKNOWN == format, base::TempString("Deth target in frame buffer is bound with format '{}' but pass layout expects it's not", format), false);
				}
			}
			else
			{
				DEBUG_CHECK_RETURN_EX_V(layout->layout().depth.format == ImageFormat::UNKNOWN, "Depth target in frame buffer is not specified but it's expected by pass layout", false);
			}

			DEBUG_CHECK_RETURN_EX_V(frameBuffer.samples() == layout->layout().samples, base::TempString("Frame buffer has {} samples per pixel while layout expects {}", frameBuffer.samples(), layout->layout().samples), false);

#ifdef VALIDATE_RESOURCE_LAYOUTS
			for (uint32_t i = 0; i < FrameBuffer::MAX_COLOR_TARGETS; ++i)
			{
				if (frameBuffer.color[i].viewPtr)
				{
					// NOTE: render target may NOT be owned by image but then the layout does not change much
					if (auto* owningImage = base::rtti_cast<ImageObject>(frameBuffer.color[i].viewPtr->object()))
						ensureResourceState(owningImage, ResourceLayout::RenderTarget);
				}
			}

			if (frameBuffer.depth.viewPtr)
			{
				// NOTE: render target may NOT be owned by image but then the layout does not change much
				if (auto* owningImage = base::rtti_cast<ImageObject>(frameBuffer.depth.viewPtr->object()))
					ensureResourceState(owningImage, ResourceLayout::DepthWrite);
			}
#endif

			return true;
		}

        void CommandWriter::opBeingPass(const GraphicsPassLayoutObject* layout, const FrameBuffer& frameBuffer, uint8_t viewportCount, const base::Rect& drawArea)
        {
			DEBUG_CHECK_RETURN_EX(layout, "Invalid layout specified");
            DEBUG_CHECK_RETURN_EX(!m_currentPass, "Recursive passes are not allowed");
			DEBUG_CHECK_RETURN_EX(viewportCount <= 16, "To many viewports");

			uint32_t width = 0, height = 0;
			DEBUG_CHECK_RETURN_EX(validateFrameBuffer(frameBuffer, layout, &width, &height), "Invalid frame buffer");          

			//--

			if (!drawArea.empty())
			{
				DEBUG_CHECK_RETURN_EX(drawArea.min.x >= 0, "Invalid draw area");
				DEBUG_CHECK_RETURN_EX(drawArea.max.x <= (int)width, "Invalid draw area");
				DEBUG_CHECK_RETURN_EX(drawArea.min.y >= 0, "Invalid draw area");
				DEBUG_CHECK_RETURN_EX(drawArea.max.y <= (int)height, "Invalid draw area");
			}

            auto op = allocCommand<OpBeginPass>();
            op->frameBuffer = frameBuffer;
			op->passLayoutId = layout->id();
            op->hasResourceTransitions = false;
			op->viewportCount = viewportCount;

			if (!drawArea.empty())
				op->renderArea = drawArea;
			else
				op->renderArea = base::Rect(0, 0, width, height);

			m_currentPassLayout = AddRef(layout);

            m_currentPass = op;
            m_currentPassRts = frameBuffer.validColorSurfaces();
			m_currentPassViewports = viewportCount;
        }

        void CommandWriter::opEndPass()
        {
            DEBUG_CHECK_RETURN(m_currentPass);// , "Not in pass");
            DEBUG_CHECK_RETURN(!m_isChildBufferWithParentPass);// , "Cannot end pass that started in parent command buffer");

            auto op = allocCommand<OpEndPass>();
			m_currentPassLayout.reset();
            m_currentPass = nullptr;
			m_currentPassViewports = 0;
			m_currentPassRts = 0;
        }

		void CommandWriter::opClearFrameBuffer(const GraphicsPassLayoutObject* layout, const FrameBuffer& frameBuffer, const base::Rect* area/* = nullptr*/)
		{
			DEBUG_CHECK_RETURN_EX(layout, "Invalid layout specified");
			DEBUG_CHECK_RETURN_EX(!m_currentPass, "Cannot clear pass inside another pass");

			uint32_t width = 0, height = 0;
			DEBUG_CHECK_RETURN_EX(validateFrameBuffer(frameBuffer, layout), "Invalid frame buffer");

			if (area)
			{
				DEBUG_CHECK_RETURN_EX(!area->empty(), "Empty clear area");
				DEBUG_CHECK_RETURN_EX(area->min.x >= 0, "Invalid clear area");
				DEBUG_CHECK_RETURN_EX(area->max.x <= (int)width, "Invalid clear area");
				DEBUG_CHECK_RETURN_EX(area->min.y >= 0, "Invalid clear area");
				DEBUG_CHECK_RETURN_EX(area->max.y <= (int)height, "Invalid clear area");
			}

			auto op = allocCommand<OpClearFrameBuffer>();
			op->frameBuffer = frameBuffer;
			if (area)
				op->customArea = *area;
			else
				op->customArea = base::Rect(0, 0, width, height);
		}

        //--

        void CommandWriter::opBeginBlock(base::StringView name, base::StringView file, uint32_t line)
        {
            auto op = allocCommand<OpBeginBlock>(name.length() + 1);

            auto* payload = op->payload<char>();
            memcpy(payload, name.data(), name.length());
            payload[name.length()] = 0;
            m_numOpenedBlocks += 1;
        }

        void CommandWriter::opEndBlock()
        {
            DEBUG_CHECK_RETURN(m_numOpenedBlocks > 0);

            auto op = allocCommand<OpEndBlock>();
            m_numOpenedBlocks -= 1;
        }

        //--

        CommandBuffer* CommandWriter::opCreateChildCommandBuffer(bool inheritParameters /*= true*/)
        {
            CommandBuffer* ret = CommandBuffer::Alloc();
            ret->m_isChildCommandBuffer = true;
            ret->m_parentBufferBeginPass = m_currentPass;
            if (inheritParameters)
                ret->m_activeParameterBindings = m_currentParameterBindings;

            auto op = allocCommand<OpChildBuffer>();
            op->childBuffer = ret;
            op->inheritsParameters = inheritParameters;
            op->insidePass = (m_currentPass != nullptr);
            op->nextChildBuffer = nullptr;

            if (m_writeBuffer->m_firstChildBuffer)
            {
                m_writeBuffer->m_lastChildBuffer->nextChildBuffer = op;
                m_writeBuffer->m_lastChildBuffer = op;
            }
            else
            {
                m_writeBuffer->m_firstChildBuffer = op;
                m_writeBuffer->m_lastChildBuffer = op;
            }

            return ret;
        }

        void CommandWriter::opAttachChildCommandBuffer(CommandBuffer* buffer)
        {
            DEBUG_CHECK_RETURN(m_currentPass == nullptr)// , "External command buffers can only be attached outside the pass");

            auto op = allocCommand<OpChildBuffer>();
            op->childBuffer = buffer;
            op->inheritsParameters = false;
            op->insidePass = false;
            op->nextChildBuffer = nullptr;

            if (m_writeBuffer->m_firstChildBuffer)
            {
                m_writeBuffer->m_lastChildBuffer->nextChildBuffer = op;
                m_writeBuffer->m_lastChildBuffer = op;
            }
            else
            {
                m_writeBuffer->m_firstChildBuffer = op;
                m_writeBuffer->m_lastChildBuffer = op;
            }
        }

        //--

        void CommandWriter::opSetViewportRect(uint8_t viewport, const base::Rect& viewportRect, float depthMin, float depthMax)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);
            DEBUG_CHECK_RETURN(viewport < m_currentPassViewports);// "Invalid viewport index, current pass defines less view ports");
            DEBUG_CHECK_RETURN(viewportRect.width() >= 0);// , "Width can't be negative");
            DEBUG_CHECK_RETURN(viewportRect.height() >= 0);// , "Height can't be negative");

            auto op = allocCommand<OpSetViewportRect>();
            op->viewportIndex = viewport;
            op->rect = viewportRect;
			op->depthMin = depthMin;
			op->depthMax = depthMax;
        }

        void CommandWriter::opSetViewportRect(uint8_t viewport, int x, int y, int w, int h, float depthMin, float depthMax)
        {
            opSetViewportRect(viewport, base::Rect(x, y, x + w, y + h), depthMin, depthMax);
        }

		void CommandWriter::opSetBlendConstant(const base::Color& color)
		{
			opSetBlendConstant(color.toVectorLinear());
		}

		void CommandWriter::opSetBlendConstant(const base::Vector4& color)
		{
			DEBUG_CHECK_RETURN(m_currentPass != nullptr);

			auto op = allocCommand<OpSetBlendColor>();
			op->color[0] = color.x;
			op->color[1] = color.y;
			op->color[2] = color.z;
			op->color[3] = color.w;
		}

		void CommandWriter::opSetBlendConstant(float r /*= 1.0f*/, float g /*= 1.0f*/, float b /*= 1.0f*/, float a /*= 1.0f*/)
		{
			DEBUG_CHECK_RETURN(m_currentPass != nullptr);

			auto op = allocCommand<OpSetBlendColor>();
			op->color[0] = r;
			op->color[1] = g;
			op->color[2] = b;
			op->color[3] = a;
		}

		void CommandWriter::opSetLineWidth(float lineWidth)
		{
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);

            auto op = allocCommand<OpSetLineWidth>();
			op->width = lineWidth;
        }

        void CommandWriter::opSetScissorRect(uint8_t viewportIndex, const base::Rect& scissorRect)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);
            DEBUG_CHECK_RETURN(viewportIndex < m_currentPassViewports);// , "Invalid viewport index, current pass defines less viewports");
            DEBUG_CHECK_RETURN(scissorRect.width() >= 0);// "Width can't be negative");
            DEBUG_CHECK_RETURN(scissorRect.height() >= 0);//, "Height can't be negative");

            auto op = allocCommand<OpSetScissorRect>();
            op->viewportIndex = viewportIndex;
            op->rect = scissorRect;
        }

        void CommandWriter::opSetScissorRect(uint8_t viewportIndex, int x, int y, int w, int h)
        {
            opSetScissorRect(viewportIndex, base::Rect(x, y, x + w, y + h));
        }

        void CommandWriter::opSetScissorBounds(uint8_t viewportIndex, int x0, int y0, int x1, int y1)
        {
            opSetScissorRect(viewportIndex, base::Rect(x0, y0, x1, y1));
        }

        void CommandWriter::opSetStencilReferenceValue(uint8_t value)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);

            auto op  = allocCommand<OpSetStencilReference>();
            op->front = value;
            op->back = value;
        }

        void CommandWriter::opSetStencilReferenceValue(uint8_t frontValue, uint8_t backValue)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);

            auto op = allocCommand<OpSetStencilReference>();
            op->front = frontValue;
            op->back = backValue;
        }

        /*void CommandWriter::opSetStencilCompareMask(uint8_t mask)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);

            auto op  = allocCommand<OpSetStencilCompareMask>();
            op->front = mask;
            op->back = mask;
        }

        void CommandWriter::opSetStencilCompareMask(uint8_t frontValue, uint8_t backValue)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);

            auto op = allocCommand<OpSetStencilCompareMask>();
            op->front = frontValue;
            op->back = backValue;
        }

        void CommandWriter::opSetStencilWriteMask(uint8_t mask)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);

            auto op  = allocCommand<OpSetStencilWriteMask>();
            op->front = mask;
            op->back = mask;
        }

        void CommandWriter::opSetStencilWriteMask(uint8_t front, uint8_t back)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);

            auto op = allocCommand<OpSetStencilWriteMask>();
            op->front = front;
            op->back = back;
        }*/

		void CommandWriter::opSetDepthClip(float minBounds, float maxBounds)
		{
			DEBUG_CHECK_RETURN(m_currentPass != nullptr);

			auto op = allocCommand<OpSetDepthClip>();
			op->min = minBounds;
			op->max = maxBounds;
		}

		/*void CommandWriter::opSetDepthBias(float constant, float slopeFactor, float clampValue)
		{
			DEBUG_CHECK_RETURN(m_currentPass != nullptr);

			auto op = allocCommand<OpSetDepthBias>();
			op->constant = constant;
			op->slope = slopeFactor;
			op->clamp = clampValue;
		}*/

        //--

        void CommandWriter::opClearPassRenderTarget(uint32_t index, const base::Vector4& color)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);
            DEBUG_CHECK_RETURN(index < m_currentPassRts);// , "Invalid render target index");
            DEBUG_CHECK_RETURN(m_currentPass->frameBuffer.color[index]);// , "Clearing color render target that was never bound");

            auto op = allocCommand<OpClearPassRenderTarget>();
            op->index = index;
            op->color[0] = color.x;
            op->color[1] = color.y;
            op->color[2] = color.z;
            op->color[3] = color.w;
        }

        void CommandWriter::opClearPassDepthStencil(float depth, uint8_t stencil, bool doClearDepth /*= true*/, bool doClearStencil /*= true*/)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);
            DEBUG_CHECK_RETURN(m_currentPass->frameBuffer.depth);// , "Clearing color render target that was never bound");

            if (doClearStencil || doClearDepth)
            {
                auto op = allocCommand<OpClearPassDepthStencil>();
                op->depthValue = depth;
                op->stencilValue = stencil;
                op->clearFlags = (doClearDepth ? 1 : 0) | (doClearStencil ? 2 : 0);
            }
        }

		void CommandWriter::opClearWritableBufferRects(const BufferWritableView* bufferView, const void* clearValue /*= nullptr*/, const ResourceClearRect* rects /*= nullptr*/, uint32_t numRects /*= 0*/)
		{
			DEBUG_CHECK_RETURN_EX(bufferView != nullptr, "Missing buffer");
			DEBUG_CHECK_RETURN_EX((rects == nullptr && numRects == 0) || (rects != nullptr && numRects >= 0), "Invalid rectangles");

			uint32_t payloadDataSize = 0;
			uint32_t clearValueSize = 0;
			uint32_t colorValuePayloadOffset = 0;

			clearValueSize = GetImageFormatInfo(bufferView->format()).bitsPerPixel / 8;
			payloadDataSize = clearValueSize;

#ifdef VALIDATE_RESOURCE_LAYOUTS
			DEBUG_CHECK_RETURN(ensureResourceState(bufferView->object(), ResourceLayout::UAV));
#endif

			if (numRects != 0)
			{
				const auto bufferSize = bufferView->size();

				for (uint32_t i = 0; i < numRects; ++i)
				{
					const auto& r = rects[i];
					DEBUG_CHECK_RETURN(r.buffer.offset < bufferSize);
					DEBUG_CHECK_RETURN(r.buffer.offset + r.buffer.size < bufferSize);

					const auto totalOffset = bufferView->offset() + r.buffer.offset;
					DEBUG_CHECK_RETURN(totalOffset % clearValueSize == 0);
					DEBUG_CHECK_RETURN(r.buffer.size % clearValueSize == 0);
				}

				colorValuePayloadOffset = sizeof(base::image::ImageRect) * numRects;
				payloadDataSize += colorValuePayloadOffset;
			}

			auto op = allocCommand<OpClearBuffer>(payloadDataSize);
			op->view = bufferView->viewId();
			op->clearFormat = bufferView->format();
			op->numRects = numRects;

			if (numRects)
				memcpy(op->payload(), rects, sizeof(base::image::ImageRect) * numRects);

			if (clearValue)
				memcpy(op->payload<uint8_t>() + colorValuePayloadOffset, clearValue, clearValueSize);
			else
				memzero(op->payload<uint8_t>() + colorValuePayloadOffset, clearValueSize);
		}

        void CommandWriter::opClearWritableBuffer(const BufferWritableView* view, const void* clearValue /*= nullptr*/, uint32_t offset /*= 0*/, uint32_t size /*= INDEX_MAX*/)
        {
			DEBUG_CHECK_RETURN(view);

			DEBUG_CHECK_RETURN(view->format() != ImageFormat::UNKNOWN);
			DEBUG_CHECK_RETURN(offset < view->size());

			if (size == INDEX_MAX)
				size = view->size() - offset;

			DEBUG_CHECK_RETURN(size > 0);
			DEBUG_CHECK_RETURN(offset + size <= view->size());

			if (offset == 0 && size == view->size())
			{
				opClearWritableBufferRects(view, clearValue, nullptr, 0);
			}
			else
			{
				ResourceClearRect rect;
				rect.buffer.offset = offset;
				rect.buffer.size = size;
				opClearWritableBufferRects(view, clearValue, &rect, 1);
			}
        }

		void CommandWriter::opClearWritableImageRects(const ImageWritableView* imageView, const void* clearValue /*= nullptr*/, const ResourceClearRect* rects /*= nullptr*/, uint32_t numRects /*= 0*/)
		{
			DEBUG_CHECK_RETURN_EX(imageView != nullptr, "Missing image");
			DEBUG_CHECK_RETURN_EX((rects == nullptr && numRects == 0) || (rects != nullptr && numRects >= 0), "Invalid rectangles");

			const auto* image = imageView->image();

			uint32_t payloadDataSize = 0;
			uint32_t clearValueSize = 0;
			uint32_t colorValuePayloadOffset = 0;

			clearValueSize = GetImageFormatInfo(image->format()).bitsPerPixel / 8;
			payloadDataSize = clearValueSize;

#ifdef VALIDATE_RESOURCE_LAYOUTS
			{
				SubImageRegion region;
				region.firstMip = imageView->mip();
				region.numMips = 1;
				region.firstSlice = imageView->slice();
				region.numSlices = 1;
				DEBUG_CHECK_RETURN(ensureResourceState(image, ResourceLayout::UAV, &region));
			}
#endif

			if (numRects != 0)
			{
				const auto mipWidth = std::max<uint32_t>(1, image->width() >> imageView->mip());
				const auto mipHeight = std::max<uint32_t>(1, image->height() >> imageView->mip());
				const auto mipDepth = std::max<uint32_t>(1, image->depth() >> imageView->mip());

				for (uint32_t i = 0; i < numRects; ++i)
				{
					const auto& r = rects[i];
					DEBUG_CHECK_RETURN(r.image.offsetX < mipWidth);
					DEBUG_CHECK_RETURN(r.image.offsetY < mipHeight);
					DEBUG_CHECK_RETURN(r.image.offsetZ < mipDepth);
					DEBUG_CHECK_RETURN(r.image.offsetX + r.image.sizeX <= mipWidth);
					DEBUG_CHECK_RETURN(r.image.offsetY + r.image.sizeY <= mipHeight);
					DEBUG_CHECK_RETURN(r.image.offsetZ + r.image.sizeZ <= mipDepth);
				}

				colorValuePayloadOffset = sizeof(base::image::ImageRect) * numRects;
				payloadDataSize += colorValuePayloadOffset;
			}

			auto op = allocCommand<OpClearImage>(payloadDataSize);
			op->view = imageView->viewId();
			op->clearFormat = image->format();
			op->numRects = numRects;

			if (numRects)
				memcpy(op->payload(), rects, sizeof(base::image::ImageRect) * numRects);

			if (clearValue)
				memcpy(op->payload<uint8_t>() + colorValuePayloadOffset, clearValue, clearValueSize);
			else
				memzero(op->payload<uint8_t>() + colorValuePayloadOffset, clearValueSize);
		}

        void CommandWriter::opClearWritableImage(const ImageWritableView* view, const void* clearValue)
        {
			DEBUG_CHECK_RETURN(view);

			const auto* image = view->image();
			DEBUG_CHECK_RETURN(image != nullptr);
			DEBUG_CHECK_RETURN(image->format() != ImageFormat::UNKNOWN);

			opClearWritableImageRects(view, clearValue, nullptr, 0);
        }

        void CommandWriter::opClearRenderTarget(const RenderTargetView * view, const base::Vector4 & values, const base::Rect * rects /*= nullptr*/, uint32_t numRects /*= 0*/)
        {
			DEBUG_CHECK_RETURN_EX(view, "Invalid render target view");
			DEBUG_CHECK_RETURN_EX(!view->depth(), "Render target view is not a color render target");
			DEBUG_CHECK_RETURN_EX(!view->swapchain(), "Cannot clear swap chain views like that");

			for (uint32_t i = 0; i < numRects; ++i)
			{
				const auto& r = rects[i];
				DEBUG_CHECK_RETURN(r.min.x >= 0);
				DEBUG_CHECK_RETURN(r.min.y >= 0);
				DEBUG_CHECK_RETURN(r.max.x <= view->width());
				DEBUG_CHECK_RETURN(r.max.y <= view->height());
			}

			const auto payloadDataSize = sizeof(base::Rect) * numRects;

#ifdef VALIDATE_RESOURCE_LAYOUTS
			{
				SubImageRegion region;
				region.firstMip = view->mip();
				region.numMips = 1;
				region.firstSlice = view->firstSlice();
				region.numSlices = view->slices();
				DEBUG_CHECK_RETURN(ensureResourceState(view->object(), ResourceLayout::RenderTarget, &region));
			}
#endif

			auto op = allocCommand<OpClearRenderTarget>(payloadDataSize);
			op->view = view->viewId();
			op->color[0] = values.x;
			op->color[1] = values.y;
			op->color[2] = values.z;
			op->color[3] = values.w;
			op->numRects = numRects;

			if (payloadDataSize)
				memcpy(op->payload(), rects, payloadDataSize);
        }

        void CommandWriter::opClearDepthStencil(const RenderTargetView* view, bool doClearDepth, bool doClearStencil, float clearDepth, uint32_t clearStencil, const base::Rect* rects /*= nullptr*/, uint32_t numRects /*= 0*/)
        {
			DEBUG_CHECK_RETURN_EX(view, "Invalid render target view");
			DEBUG_CHECK_RETURN_EX(view->depth(), "Render target view is not a depth buffer");
			DEBUG_CHECK_RETURN_EX(!view->swapchain(), "Cannot clear swap chain views like that");

			for (uint32_t i = 0; i < numRects; ++i)
			{
				const auto& r = rects[i];
				DEBUG_CHECK_RETURN(r.min.x >= 0);
				DEBUG_CHECK_RETURN(r.min.y >= 0);
				DEBUG_CHECK_RETURN(r.max.x <= view->width());
				DEBUG_CHECK_RETURN(r.max.y <= view->height());
			}

#ifdef VALIDATE_RESOURCE_LAYOUTS
			{
				SubImageRegion region;
				region.firstMip = view->mip();
				region.numMips = 1;
				region.firstSlice = view->firstSlice();
				region.numSlices = view->slices();
				DEBUG_CHECK_RETURN(ensureResourceState(view->object(), ResourceLayout::DepthWrite, &region));
			}
#endif

			const auto payloadDataSize = sizeof(base::Rect) * numRects;

			auto op = allocCommand<OpClearDepthStencil>(payloadDataSize);
			op->view = view->viewId();
			op->stencilValue = clearStencil;
			op->depthValue = clearDepth;
			op->numRects = numRects;
			op->clearFlags = 0;
			if (doClearDepth)
				op->clearFlags = 1;
			if (doClearStencil)
				op->clearFlags |= 1;

			if (payloadDataSize)
				memcpy(op->payload(), rects, payloadDataSize);
        }

		static bool FormatsComaptibleForResolve(ImageFormat src, ImageFormat dest)
		{
			// TODO: some formats can be resolved R32F -> R32UI, etc
			return src == dest;
		}

        void CommandWriter::opResolve(const ImageObject* msaaSource, const ImageObject* nonMsaaDest, uint8_t sourceMip /*= 0*/, uint8_t destMip /*= 0*/, uint16_t sourceSlice /*= 0*/, uint16_t destSlice /*= 0*/)
        {
            DEBUG_CHECK_RETURN_EX(msaaSource , "Invalid source");
            DEBUG_CHECK_RETURN_EX(nonMsaaDest, "Invalid destination");
			DEBUG_CHECK_RETURN_EX(msaaSource != nonMsaaDest, "Can resolve to the same resource");
            //DEBUG_CHECK_RETURN_EX(msaaSource->multisampled() && msaaSource->samples() > 1, "Source should be multisampled");
            DEBUG_CHECK_RETURN_EX(!nonMsaaDest->multisampled(), "Destination should not be multisampled");
            DEBUG_CHECK_RETURN_EX(FormatsComaptibleForResolve(msaaSource->format(), nonMsaaDest->format()), "Source and Destination must have the same format");
            DEBUG_CHECK_RETURN_EX(msaaSource->width() == nonMsaaDest->width() && msaaSource->height() == nonMsaaDest->height(), "Source and Destination must have the same size");

			DEBUG_CHECK_RETURN_EX(msaaSource->type() == ImageViewType::View2D || msaaSource->type() == ImageViewType::View2DArray, "Only 2D images are supported for resolve");
			DEBUG_CHECK_RETURN_EX(nonMsaaDest->type() == ImageViewType::View2D || nonMsaaDest->type() == ImageViewType::View2DArray, "Only 2D images are supported for resolve");

			DEBUG_CHECK_RETURN_EX(sourceMip < msaaSource->mips(), "Invalid source mip index");
			DEBUG_CHECK_RETURN_EX(destMip < nonMsaaDest->mips(), "Invalid dest mip index");
			DEBUG_CHECK_RETURN_EX(sourceSlice < msaaSource->slices(), "Invalid source mip index");
			DEBUG_CHECK_RETURN_EX(destSlice < nonMsaaDest->slices(), "Invalid dest mip index");


#ifdef VALIDATE_RESOURCE_LAYOUTS
			{
				SubImageRegion srcRegion;
				srcRegion.firstMip = sourceMip;
				srcRegion.firstSlice = sourceSlice;
				srcRegion.numMips = 1;
				srcRegion.numSlices = 1;
				DEBUG_CHECK_RETURN(ensureResourceState(msaaSource, ResourceLayout::ResolveSource, &srcRegion));

				SubImageRegion destRegion;
				destRegion.firstMip = destMip;
				destRegion.firstSlice = destSlice;
				destRegion.numMips = 1;
				destRegion.numSlices = 1;
				DEBUG_CHECK_RETURN(ensureResourceState(nonMsaaDest, ResourceLayout::ResolveDest, &destRegion));
			}
#endif

            auto op = allocCommand<OpResolve>();
            op->source = msaaSource->id();
            op->dest = nonMsaaDest->id();
			op->sourceMip = sourceMip;
			op->sourceSlice = sourceSlice;
			op->destMip = destMip;
			op->destSlice = destSlice;            
        }

        void CommandWriter::opBindVertexBuffer(base::StringID bindpoint, const BufferObject* buffer, uint32_t offset /*= 0*/)
        {
            DEBUG_CHECK_RETURN(bindpoint);// , "Invalid bind point");
            DEBUG_CHECK_RETURN(buffer);// .id(), "Invalid vertex buffer");
            DEBUG_CHECK_RETURN(buffer->vertex());// , "Buffer was not created with inteded use as vertex buffer");
            DEBUG_CHECK_RETURN(offset < buffer->size());// , "Offset to vertex data is not within the buffer");

            auto op = allocCommand<OpBindVertexBuffer>();
            op->bindpoint = bindpoint;
            op->offset = offset;
            op->id = buffer->id();

#ifdef VALIDATE_VERTEX_LAYOUTS
            m_currentVertexBufferRemainingSize[bindpoint] = buffer->size() - offset;
			m_currentVertexBuffers[bindpoint] = AddRef(buffer);
#endif
        }

        void CommandWriter::opUnbindVertexBuffer(base::StringID bindpoint)
        {
            DEBUG_CHECK_RETURN(bindpoint);// , "Invalid bind point");

            auto op = allocCommand<OpBindVertexBuffer>();
            op->bindpoint = bindpoint;
            op->offset = 0;
            op->id = ObjectID();

#ifdef VALIDATE_VERTEX_LAYOUTS
            m_currentVertexBufferRemainingSize[bindpoint] = 0;
			m_currentVertexBuffers[bindpoint] = nullptr;
#endif
        }

        void CommandWriter::opBindIndexBuffer(const BufferObject* buffer, ImageFormat indexFormat, uint32_t offset /*= 0*/)
        {
            DEBUG_CHECK_RETURN(buffer);// .id(), "Invalid index buffer");
            DEBUG_CHECK_RETURN(buffer->index());// "Buffer was not created with intended use as index buffer");
            DEBUG_CHECK_RETURN(indexFormat == ImageFormat::R32_UINT || indexFormat == ImageFormat::R16_UINT); // , "Invalid index buffer format");
            DEBUG_CHECK_RETURN(indexFormat != ImageFormat::R32_UINT || (0 == (offset % 4))); // , "Index data must be aligned");
            DEBUG_CHECK_RETURN(indexFormat != ImageFormat::R16_UINT || (0 == (offset % 2))); // , "Index data must be aligned");
            DEBUG_CHECK_RETURN(offset < buffer->size());// , "Offset to index data is not within the buffer");

            auto op = allocCommand<OpBindIndexBuffer>();
            op->offset = offset;
            op->id = buffer->id();
            op->format = indexFormat;

#ifdef VALIDATE_VERTEX_LAYOUTS
            if (indexFormat == ImageFormat::R32_UINT)
                m_currentIndexBufferElementCount = (buffer->size() - offset) / 4;
            else
                m_currentIndexBufferElementCount = (buffer->size() - offset) / 2;

			m_currentIndexBuffer = AddRef(buffer);
#endif
        }

        void CommandWriter::opUnbindIndexBuffer()
        {
            auto op  = allocCommand<OpBindIndexBuffer>();
            op->offset = 0;
            op->format = ImageFormat::R32_UINT;
            op->id = ObjectID();

#ifdef VALIDATE_VERTEX_LAYOUTS
            m_currentIndexBufferElementCount = 0;
			m_currentIndexBuffer.reset();
#endif
        }

		bool CommandWriter::validateDrawIndexLayout(uint32_t requiredElementCount)
		{
#ifdef VALIDATE_VERTEX_LAYOUTS
			DEBUG_CHECK_RETURN_EX_V(m_currentIndexBuffer, "No index buffer bound", false);
			DEBUG_CHECK_RETURN_EX_V(requiredElementCount <= m_currentIndexBufferElementCount, "Not enough index elements in the buffer", false);

#ifdef VALIDATE_RESOURCE_LAYOUTS
			DEBUG_CHECK_RETURN_V(ensureResourceState(m_currentIndexBuffer, ResourceLayout::IndexBuffer), false);
#endif

#endif
			return true;
		}

        bool CommandWriter::validateDrawVertexLayout(const ShaderMetadata* meta, uint32_t requiredVertexCount, uint32_t requiredInstanceCount)
        {
#ifdef VALIDATE_VERTEX_LAYOUTS
			DEBUG_CHECK_RETURN_V(meta, false);

            // check that we have a vertex buffer for each referenced binding point
            for (const auto& stream : meta->vertexStreams)
            {
                uint32_t dataSizeAtVertexBinding = 0;
                bool found = m_currentVertexBufferRemainingSize.find(stream.name, dataSizeAtVertexBinding);
				DEBUG_CHECK_RETURN_EX_V(found, base::TempString("Missing vertex buffer '{}' for current draw call", stream.name), false);
                    
				if (stream.instanced)
				{
					auto sizeNeeded = stream.size * requiredInstanceCount;
					DEBUG_CHECK_RETURN_EX_V(dataSizeAtVertexBinding >= sizeNeeded, base::TempString("Instance vertex buffer '{}' has not enough data for current draw call {} < {}", stream.name, dataSizeAtVertexBinding, sizeNeeded), false);
				}
				else
				{
					auto sizeNeeded = stream.size * requiredVertexCount;
					DEBUG_CHECK_RETURN_EX_V(dataSizeAtVertexBinding >= sizeNeeded, base::TempString("Vertex buffer '{}' has not enough data for current draw call {} < {}", stream.name, dataSizeAtVertexBinding, sizeNeeded), false);
				}

#ifdef VALIDATE_RESOURCE_LAYOUTS
				{
					const auto buffer = m_currentVertexBuffers.find(stream.name);
					DEBUG_CHECK_RETURN_V(buffer != nullptr, false);
					DEBUG_CHECK_RETURN_V(ensureResourceState(buffer->get(), ResourceLayout::VertexBuffer), false);
				}
#endif
            }
#endif

            return true;
        }

        bool CommandWriter::validateParameterBindings(const ShaderMetadata* meta)
        {
#ifdef VALIDATE_DESCRIPTOR_BINDINGS
			DEBUG_CHECK_RETURN_V(meta, false);

			for (const auto& desc : meta->descriptors)
            {
                const auto* currentLayout = m_currentParameterBindings.find(desc.name);
				DEBUG_CHECK_RETURN_EX_V(currentLayout != nullptr, base::TempString("Selected shader requires descriptor '{}' that is not bound", desc.name), false);

                DEBUG_CHECK_RETURN_EX_V(currentLayout->layout().size() == desc.elements.size(), base::TempString("Bound parameter layout '{}' requires {} elements but {} provided",
					desc.name, desc.elements.size(), currentLayout->layout().size()), false);

				for (auto i : desc.elements.indexRange())
				{
					const auto& expectedElem = desc.elements[i];
					const auto& boundElemViewType = currentLayout->layout()[i];

 					DEBUG_CHECK_RETURN_EX_V(expectedElem.type == boundElemViewType, base::TempString("Element '{}' in descriptor '{}' is expected to be {} but in layout it's {}",
						expectedElem.name, desc.name, expectedElem.type, boundElemViewType), false);
				}

#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES
				const auto* descriptorEntries = (const DescriptorEntry*)m_currentParameterData.findSafe(desc.name, nullptr);
				DEBUG_CHECK_RETURN_EX_V(descriptorEntries != nullptr, base::TempString("No actual descript data for descriptor '{}' found", desc.name), false);

				for (auto i : desc.elements.indexRange())
				{
					const auto& expectedElem = desc.elements[i];
					const auto& descriptorEntry = descriptorEntries[i];

					DEBUG_CHECK_RETURN_EX_V(expectedElem.type == descriptorEntry.type, base::TempString("Element '{}' in descriptor '{}' is expected to be {} bound in data it's {}",
						expectedElem.name, desc.name, expectedElem.type, descriptorEntry.type), false);

					switch (expectedElem.type)
					{
						case DeviceObjectViewType::ConstantBuffer:
						{
							if (descriptorEntry.inlinedConstants.sourceDataPtr) // way more common to have inlined constants
							{
								DEBUG_CHECK_RETURN_V(!descriptorEntry.id, false);
								DEBUG_CHECK_RETURN_EX_V(descriptorEntry.size == expectedElem.number, base::TempString("Constant buffer '{}' in descriptor '{}' is expected to be of size {} but {} bytes were given.",
									expectedElem.name, desc.name, expectedElem.number, descriptorEntry.size), false);
							}
							else
							{
								const auto* view = base::rtti_cast<BufferConstantView>(descriptorEntry.viewPtr);
								DEBUG_CHECK_RETURN_V(view != nullptr, false);
								DEBUG_CHECK_RETURN_V(descriptorEntry.size == 0, false);
								DEBUG_CHECK_RETURN_V(descriptorEntry.offset == 0, false);

								DEBUG_CHECK_RETURN_EX_V(view->size() >= expectedElem.number, base::TempString("Constant buffer '{}' in descriptor '{}' is expected to be of size {} but {} bytes are in the bound view.",
									expectedElem.name, desc.name, expectedElem.number, view->size()), false);
							}

							break;
						}

						case DeviceObjectViewType::Buffer:
						{
							DEBUG_CHECK_RETURN_V(descriptorEntry.id, false);
							DEBUG_CHECK_RETURN_V(descriptorEntry.viewPtr, false);

							const auto* view = base::rtti_cast<BufferView>(descriptorEntry.viewPtr);
							DEBUG_CHECK_RETURN_V(view != nullptr, false);

							DEBUG_CHECK_RETURN_EX_V(expectedElem.format == view->format(), base::TempString("Buffer view '{}' in descriptor '{}' is expected to be of format '{}' but '{}' is bound.",
								expectedElem.name, desc.name, expectedElem.format, view->format()), false);

#ifdef VALIDATE_RESOURCE_LAYOUTS
							DEBUG_CHECK_RETURN_V(ensureResourceState(view->buffer(), ResourceLayout::ShaderResource), false);
#endif
							break;
						}

						case DeviceObjectViewType::BufferWritable:
						{
							DEBUG_CHECK_RETURN_V(descriptorEntry.id, false);
							DEBUG_CHECK_RETURN_V(descriptorEntry.viewPtr, false);

							const auto* view = base::rtti_cast<BufferWritableView>(descriptorEntry.viewPtr);
							DEBUG_CHECK_RETURN_V(view != nullptr, false);

							DEBUG_CHECK_RETURN_EX_V(expectedElem.format == view->format(), base::TempString("Writable buffer view '{}' in descriptor '{}' is expected to be of format '{}' but '{}' is bound.",
								expectedElem.name, desc.name, expectedElem.format, view->format()), false);

#ifdef VALIDATE_RESOURCE_LAYOUTS
							DEBUG_CHECK_RETURN_V(ensureResourceState(view->buffer(), ResourceLayout::UAV), false);
#endif
							break;
						}

						case DeviceObjectViewType::BufferStructured:
						{
							DEBUG_CHECK_RETURN_V(descriptorEntry.id, false);
							DEBUG_CHECK_RETURN_V(descriptorEntry.viewPtr, false);

							const auto* view = base::rtti_cast<BufferStructuredView>(descriptorEntry.viewPtr);
							DEBUG_CHECK_RETURN_V(view != nullptr, false);

							DEBUG_CHECK_RETURN_EX_V(expectedElem.number == view->stride(), base::TempString("Structured buffer view '{}' in descriptor '{}' is expected to have stride {} but buffer with stride {} is bound.",
								expectedElem.name, desc.name, expectedElem.number, view->stride()), false);

#ifdef VALIDATE_RESOURCE_LAYOUTS
							DEBUG_CHECK_RETURN_V(ensureResourceState(view->buffer(), ResourceLayout::ShaderResource), false);
#endif
							break;
						}

						case DeviceObjectViewType::BufferStructuredWritable:
						{
							DEBUG_CHECK_RETURN_V(descriptorEntry.id, false);
							DEBUG_CHECK_RETURN_V(descriptorEntry.viewPtr, false);

							const auto* view = base::rtti_cast<BufferWritableStructuredView>(descriptorEntry.viewPtr);
							DEBUG_CHECK_RETURN_V(view != nullptr, false);

							DEBUG_CHECK_RETURN_EX_V(expectedElem.number == view->stride(), base::TempString("Writable structured buffer view '{}' in descriptor '{}' is expected to have stride {} but buffer with stride {} is bound.",
								expectedElem.name, desc.name, expectedElem.number, view->stride()), false);

#ifdef VALIDATE_RESOURCE_LAYOUTS
							DEBUG_CHECK_RETURN_V(ensureResourceState(view->buffer(), ResourceLayout::UAV), false);
#endif
							break;
						}

						case DeviceObjectViewType::Sampler:
						{
							DEBUG_CHECK_RETURN_V(descriptorEntry.id, false);

							const auto* sampler = base::rtti_cast<SamplerObject>(descriptorEntry.objectPtr);
							DEBUG_CHECK_RETURN_V(sampler != nullptr, false);

							break;
						}

						case DeviceObjectViewType::Image:
						{
							DEBUG_CHECK_RETURN_V(descriptorEntry.id, false);
							DEBUG_CHECK_RETURN_V(descriptorEntry.viewPtr, false);

							const auto* view = base::rtti_cast<ImageReadOnlyView>(descriptorEntry.viewPtr);
							DEBUG_CHECK_RETURN_V(view != nullptr, false);

							DEBUG_CHECK_RETURN_EX_V(expectedElem.format == view->image()->format(), base::TempString("Writable image view '{}' in descriptor '{}' is expected to be of format '{}' but '{}' is bound.",
								expectedElem.name, desc.name, expectedElem.format, view->image()->format()), false);

							DEBUG_CHECK_RETURN_EX_V(expectedElem.viewType == view->image()->type(), base::TempString("Image view '{}' in descriptor '{}' is expected to be '{}' but '{}' is bound.",
								expectedElem.name, desc.name, expectedElem.type, view->image()->type()), false);

#ifdef VALIDATE_RESOURCE_LAYOUTS
							SubImageRegion region;
							region.firstMip = view->mip();
							region.numMips = 1;
							region.firstSlice = view->slice();
							region.numSlices = 1;
							DEBUG_CHECK_RETURN_V(ensureResourceState(view->image(), ResourceLayout::ShaderResource, &region), false);
#endif
							break;
						}

						case DeviceObjectViewType::ImageWritable:
						{
							DEBUG_CHECK_RETURN_V(descriptorEntry.id, false);
							DEBUG_CHECK_RETURN_V(descriptorEntry.viewPtr, false);

							const auto* view = base::rtti_cast<ImageWritableView>(descriptorEntry.viewPtr);
							DEBUG_CHECK_RETURN_V(view != nullptr, false);

							DEBUG_CHECK_RETURN_EX_V(expectedElem.format == view->image()->format(), base::TempString("Writable image view '{}' in descriptor '{}' is expected to be of format '{}' but '{}' is bound.",
								expectedElem.name, desc.name, expectedElem.format, view->image()->format()), false);

							DEBUG_CHECK_RETURN_EX_V(expectedElem.viewType == view->image()->type(), base::TempString("Writable image view '{}' in descriptor '{}' is expected to be '{}' but '{}' is bound.",
								expectedElem.name, desc.name, expectedElem.type, view->image()->type()), false);

#ifdef VALIDATE_RESOURCE_LAYOUTS
							SubImageRegion region;
							region.firstMip = view->mip();
							region.numMips = 1;
							region.firstSlice = view->slice();
							region.numSlices = 1;
							DEBUG_CHECK_RETURN_V(ensureResourceState(view->image(), ResourceLayout::UAV, &region), false);
#endif
							break;
						}

						case DeviceObjectViewType::SampledImage:
						{
							DEBUG_CHECK_RETURN_V(descriptorEntry.id, false);
							DEBUG_CHECK_RETURN_V(descriptorEntry.viewPtr, false);

							const auto* view = base::rtti_cast<ImageSampledView>(descriptorEntry.viewPtr);
							DEBUG_CHECK_RETURN_V(view != nullptr, false);

							DEBUG_CHECK_RETURN_EX_V(expectedElem.viewType == view->image()->type(), base::TempString("Image view '{}' in descriptor '{}' is expected to be '{}' but '{}' is bound.",
								expectedElem.name, desc.name, expectedElem.viewType, view->image()->type()), false);

#ifdef VALIDATE_RESOURCE_LAYOUTS
							SubImageRegion region;
							region.firstMip = view->firstMip();
							region.numMips = view->mips();
							region.firstSlice = view->firstSlice();
							region.numSlices = view->slices();
							DEBUG_CHECK_RETURN_V(ensureResourceState(view->image(), ResourceLayout::ShaderResource, &region), false);
#endif
							break;
						}

					}
				}


#endif
            }
#endif

            return true;
        }

        void CommandWriter::opDraw(const GraphicsPipelineObject* po, uint32_t firstVertex, uint32_t vertexCount)
        {
            opDrawInstanced(po, firstVertex, vertexCount, 0, 1);
        }

        void CommandWriter::opDrawInstanced(const GraphicsPipelineObject* po, uint32_t firstVertex, uint32_t vertexCount, uint16_t firstInstance, uint16_t numInstances)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);
            DEBUG_CHECK_RETURN(po);
            DEBUG_CHECK_RETURN(vertexCount > 0);
            DEBUG_CHECK_RETURN(numInstances > 0);

			const auto* metadata = po->shaders()->metadata();
            if (validateDrawVertexLayout(metadata, vertexCount, numInstances) && validateParameterBindings(metadata))
            {
                auto op = allocCommand<OpDraw>();
                op->pipelineObject = po->id();
                op->firstVertex = firstVertex;
                op->vertexCount = vertexCount;
                op->firstInstance = firstInstance;
                op->numInstances = numInstances;
            }
        }

        void CommandWriter::opDrawIndexed(const GraphicsPipelineObject* po, uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount)
        {
            opDrawIndexedInstanced(po, firstVertex, firstIndex, indexCount, 0, 1);
        }

        void CommandWriter::opDrawIndexedInstanced(const GraphicsPipelineObject* po, uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount, uint16_t firstInstance, uint16_t numInstances)
        {
            DEBUG_CHECK_RETURN(m_currentPass != nullptr);
            DEBUG_CHECK_RETURN(po);
            DEBUG_CHECK_RETURN(indexCount > 0);
            DEBUG_CHECK_RETURN(numInstances > 0);

			const auto* metadata = po->shaders()->metadata();
            if (validateDrawVertexLayout(metadata, 0, numInstances) && validateDrawIndexLayout(indexCount) && validateParameterBindings(metadata))
            {
                auto op = allocCommand<OpDrawIndexed>();
                op->pipelineObject = po->id();
                op->firstVertex = firstVertex;
                op->firstIndex = firstIndex;
                op->indexCount = indexCount;
                op->firstInstance = firstInstance;
                op->numInstances = numInstances;
            }
        }

        void CommandWriter::opDispatchGroups(const ComputePipelineObject* co, uint32_t countX /*= 1*/, uint32_t countY /*= 1*/, uint32_t countZ /*= 1*/)
        {
            DEBUG_CHECK_RETURN(co);
            DEBUG_CHECK_RETURN(countX > 0 && countY > 0 && countZ > 0);

            if (validateParameterBindings(co->shaders()->metadata()))
            {
                auto op = allocCommand<OpDispatch>();
                op->pipelineObject = co->id();
                op->counts[0] = countX;
                op->counts[1] = countY;
                op->counts[2] = countZ;
            }
        }

		void CommandWriter::opDispatchThreads(const ComputePipelineObject* co, uint32_t threadCountX /*= 1*/, uint32_t threadCountY /*= 1*/, uint32_t threadCountZ /*= 1*/)
		{
			DEBUG_CHECK_RETURN(co);
			DEBUG_CHECK_RETURN(threadCountX > 0 && threadCountY > 0 && threadCountZ > 0);

			if (validateParameterBindings(co->shaders()->metadata()))
			{
				auto op = allocCommand<OpDispatch>();
				op->pipelineObject = co->id();
				op->counts[0] = base::Align<uint32_t>(threadCountX, co->groupSizeX()) / co->groupSizeX();
				op->counts[1] = base::Align<uint32_t>(threadCountY, co->groupSizeY()) / co->groupSizeY();
				op->counts[2] = base::Align<uint32_t>(threadCountZ, co->groupSizeZ()) / co->groupSizeZ();
			}
		}

		//--

		bool CommandWriter::validateIndirectDraw(const BufferObject* buffer, uint32_t offsetInBuffer, uint32_t commandStride)
		{
			DEBUG_CHECK_RETURN_EX_V(buffer != nullptr, "No indirect arguments buffer", false);
			DEBUG_CHECK_RETURN_EX_V(buffer->indirectArgs(), "Buffer is not enabled for indirect rendering", false);
			DEBUG_CHECK_RETURN_EX_V(buffer->stride() == commandStride, "Buffer stride does not match the command stride", false);
			DEBUG_CHECK_RETURN_EX_V((offsetInBuffer % commandStride) == 0, "Indirect commands are not aligned in the buffer", false); 

			const auto sizeLeft = buffer->size() - offsetInBuffer;
			DEBUG_CHECK_RETURN_EX_V(sizeLeft >= commandStride, "Not enough space left in buffer for a full command", false);


#ifdef VALIDATE_RESOURCE_LAYOUTS
			DEBUG_CHECK_RETURN_V(ensureResourceState(buffer, ResourceLayout::IndirectArgument), false);
#endif

			return true;
		}

		void CommandWriter::opDrawIndirect(const GraphicsPipelineObject* po, const BufferObject* buffer, uint32_t offsetInBuffer)
		{
			DEBUG_CHECK_RETURN(m_currentPass != nullptr);
			DEBUG_CHECK_RETURN(po);

			const auto* metadata = po->shaders()->metadata();
			if (validateDrawVertexLayout(metadata, 0, 0) && validateParameterBindings(metadata) 
				&& validateIndirectDraw(buffer, offsetInBuffer, sizeof(GPUDrawArguments)))
			{
				auto op = allocCommand<OpDrawIndirect>();
				op->pipelineObject = po->id();
				op->argumentBuffer = buffer->id();
				op->offset = offsetInBuffer;				
			}
		}

		void CommandWriter::opDrawIndexedIndirect(const GraphicsPipelineObject* po, const BufferObject* buffer, uint32_t offsetInBuffer)
		{
			DEBUG_CHECK_RETURN(m_currentPass != nullptr);
			DEBUG_CHECK_RETURN(po);

			const auto* metadata = po->shaders()->metadata();
			if (validateDrawVertexLayout(metadata, 0, 0) && validateParameterBindings(metadata)
				&& validateDrawIndexLayout(0) && validateIndirectDraw(buffer, offsetInBuffer, sizeof(GPUDrawIndexedArguments)))
			{
				auto op = allocCommand<OpDrawIndexedIndirect>();
				op->pipelineObject = po->id();
				op->argumentBuffer = buffer->id();
				op->offset = offsetInBuffer;
			}
		}

		void CommandWriter::opDispatchGroupsIndirect(const ComputePipelineObject* po, const BufferObject* buffer, uint32_t offsetInBuffer)
		{
			DEBUG_CHECK_RETURN(m_currentPass != nullptr);
			DEBUG_CHECK_RETURN(po);

			const auto* metadata = po->shaders()->metadata();
			if (validateParameterBindings(metadata) && validateIndirectDraw(buffer, offsetInBuffer, sizeof(GPUDispatchArguments)))
			{
				auto op = allocCommand<OpDispatchIndirect>();
				op->pipelineObject = po->id();
				op->argumentBuffer = buffer->id();
				op->offset = offsetInBuffer;
			}
		}

        //--

		void CommandWriter::opTransitionFlushUAV(const ImageWritableView* imageView)
		{
			DEBUG_CHECK_RETURN(imageView);

#ifdef VALIDATE_RESOURCE_LAYOUTS
			{
				SubImageRegion region;
				region.firstMip = imageView->mip();
				region.numMips = 1;
				region.firstSlice = imageView->slice();
				region.numSlices = 1;
				DEBUG_CHECK_RETURN(ensureResourceState(imageView->image(), ResourceLayout::UAV, &region));
			}
#endif

			auto op = allocCommand<OpUAVBarrier>();
			op->viewId = imageView->viewId();
		}

		void CommandWriter::opTransitionFlushUAV(const BufferWritableView* bufferView)
		{
			DEBUG_CHECK_RETURN(bufferView);

#ifdef VALIDATE_RESOURCE_LAYOUTS
			DEBUG_CHECK_RETURN(ensureResourceState(bufferView->buffer(), ResourceLayout::UAV, nullptr));
#endif

			auto op = allocCommand<OpUAVBarrier>();
			op->viewId = bufferView->viewId();
		}

#ifdef VALIDATE_RESOURCE_LAYOUTS
		static bool LayoutCompatible(const IDeviceObject* obj, ResourceLayout incomingLayout)
		{
			if (const auto* buffer = base::rtti_cast<BufferObject>(obj))
			{
				switch (incomingLayout)
				{
				case ResourceLayout::Common:
				case ResourceLayout::ConstantBuffer:
				case ResourceLayout::VertexBuffer:
				case ResourceLayout::IndexBuffer:
				case ResourceLayout::UAV:
				case ResourceLayout::ShaderResource:
				case ResourceLayout::IndirectArgument:
				case ResourceLayout::CopyDest:
				case ResourceLayout::CopySource:
				case ResourceLayout::RayTracingAcceleration:
					return true;
				}
			}
			else if (const auto* image = base::rtti_cast<ImageObject>(obj))
			{
				switch (incomingLayout)
				{
				case ResourceLayout::Common:
				case ResourceLayout::UAV:
				case ResourceLayout::ShaderResource:
				case ResourceLayout::CopyDest:
				case ResourceLayout::CopySource:
					return true;

				case ResourceLayout::RenderTarget:
					return image->renderTarget() && !image->renderTargetDepth();

				case ResourceLayout::DepthWrite:
				case ResourceLayout::DepthRead:
					return image->renderTarget() && image->renderTargetDepth();

				case ResourceLayout::ResolveDest:
					return image->samples() == 1;

				case ResourceLayout::ResolveSource:
					return image->renderTarget();
				}
			}

			ASSERT(!"Unknown object type for layout tracking");
			return false;
		}

		ResourceCurrentStateTrackingRecord* CommandWriter::createResourceStateTrackingEntry(const IDeviceObject* obj)
		{
			if (const auto* imageObject = base::rtti_cast<ImageObject>(obj))
			{
				if (imageObject->subResourceLayouts())
				{
					const auto entryCount = imageObject->mips() * imageObject->slices();
					const auto memorySize = sizeof(ResourceCurrentStateTrackingRecord) + (entryCount - 1) * sizeof(ResourceCurrentStateTrackingRecord::SubResourceState);

					auto* entry = (ResourceCurrentStateTrackingRecord*)allocMemory(memorySize);
					memzero(entry, memorySize);

					entry->numSubResources = entryCount;
					entry->tracksSubResources = true;
					entry->numImageMips = imageObject->mips();
					entry->numImageSlices = imageObject->slices();

					const auto initialLayout = imageObject->initialLayout();
					if (initialLayout != ResourceLayout::INVALID)
					{
						for (uint32_t i = 0; i < entryCount; ++i)
						{
							entry->subResources[i].currentLayout = initialLayout;
							entry->subResources[i].firstKnownLayout = initialLayout;
						}
					}

					return entry;
				}
				else
				{
					auto* entry = (ResourceCurrentStateTrackingRecord*)allocMemory(sizeof(ResourceCurrentStateTrackingRecord));
					memzero(entry, sizeof(ResourceCurrentStateTrackingRecord));
					entry->numSubResources = 1;
					entry->tracksSubResources = false;
					entry->numImageMips = imageObject->mips();
					entry->numImageSlices = imageObject->slices();

					const auto initialLayout = imageObject->initialLayout();
					if (initialLayout != ResourceLayout::INVALID)
					{
						entry->subResources[0].currentLayout = initialLayout;
						entry->subResources[0].firstKnownLayout = initialLayout;
					}

					return entry;
				}
			}
			else if (const auto* bufferObject = base::rtti_cast<BufferObject>(obj))
			{
				auto* entry = (ResourceCurrentStateTrackingRecord*)allocMemory(sizeof(ResourceCurrentStateTrackingRecord));
				memzero(entry, sizeof(ResourceCurrentStateTrackingRecord));
				entry->numSubResources = 1;
				entry->tracksSubResources = false;

				const auto initialLayout = bufferObject->initialLayout();
				if (initialLayout != ResourceLayout::INVALID)
				{
					entry->subResources[0].currentLayout = initialLayout;
					entry->subResources[0].firstKnownLayout = initialLayout;
				}

				return entry;
			}

			ASSERT(!"Unknown object type for layout tracking");
			return nullptr;
		}

		bool CommandWriter::ensureResourceState(const IDeviceObject* object, ResourceLayout layout, const SubImageRegion* subImageRegion, ResourceLayout newLayout)
		{
			DEBUG_CHECK_RETURN_V(object, false);
			DEBUG_CHECK_RETURN_V(layout != ResourceLayout::Common && layout != ResourceLayout::INVALID, false);

			// create tracking
			ResourceCurrentStateTrackingRecord* entry = nullptr;
			if (!m_currentResourceState.find(object->id(), entry))
			{
				entry = createResourceStateTrackingEntry(object);
				m_currentResourceState[object->id()] = entry;
			}

			// make sure resource allows sub-resource tracking
			if (subImageRegion)
			{
				DEBUG_CHECK_RETURN_V(subImageRegion->firstMip < entry->numImageMips, false);
				DEBUG_CHECK_RETURN_V(subImageRegion->firstSlice < entry->numImageSlices, false);
				DEBUG_CHECK_RETURN_V(subImageRegion->firstMip + subImageRegion->numMips <= entry->numImageMips, false);
				DEBUG_CHECK_RETURN_V(subImageRegion->firstSlice + subImageRegion->numSlices <= entry->numImageSlices, false);
			}

			if (entry->tracksSubResources && subImageRegion)
			{
				auto entryIndex = entry->numImageMips * subImageRegion->firstSlice;
				for (uint32_t i = 0; i < subImageRegion->numSlices; ++i)
				{
					auto localEntryIndex = entryIndex + subImageRegion->firstMip;
					for (uint32_t j = 0; j < subImageRegion->numMips; ++j, ++localEntryIndex)
					{
						auto& layoutEntry = entry->subResources[localEntryIndex];
						if (layoutEntry.currentLayout == ResourceLayout::INVALID)
						{
							layoutEntry.currentLayout = layout;
							layoutEntry.firstKnownLayout = layout;
						}
						else
						{
							DEBUG_CHECK_RETURN_EX_V(layoutEntry.currentLayout == layout, base::TempString("Sub-Resource {},{} of {} should be in layout {} but is in {}",
								i, j, object->id(), layout, layoutEntry.currentLayout), false);
						}

						if (newLayout != ResourceLayout::INVALID)
							layoutEntry.currentLayout = newLayout;
					}

					entryIndex += entry->numImageMips;
				}
			}
			else
			{
				if (subImageRegion && !entry->tracksSubResources)
				{
					DEBUG_CHECK_RETURN_EX_V(newLayout == ResourceLayout::INVALID, "Trying to change layout of a sub-resource on a resource that is not tracking that", false);
				}

				for (uint32_t i = 0; i < entry->numSubResources; ++i)
				{
					auto& layoutEntry = entry->subResources[i];
					if (layoutEntry.currentLayout == ResourceLayout::INVALID)
					{
						layoutEntry.currentLayout = layout;
						layoutEntry.firstKnownLayout = layout;
					}
					else
					{
						DEBUG_CHECK_RETURN_EX_V(layoutEntry.currentLayout == layout, base::TempString("Sub-Resource {} of {} should be in layout {} but is in {}", 
							i, object->id(), layout, layoutEntry.currentLayout), false);
					}

					if (newLayout != ResourceLayout::INVALID)
						layoutEntry.currentLayout = newLayout;
				}
			}

			return true;
		}
#endif

		void CommandWriter::opTransitionLayout(const IDeviceObject* obj, ResourceLayout incomingLayout, ResourceLayout outgoingLayout)
		{
			DEBUG_CHECK_RETURN(obj != nullptr);
			DEBUG_CHECK_RETURN(incomingLayout != outgoingLayout);

#ifdef VALIDATE_RESOURCE_LAYOUTS
			DEBUG_CHECK_RETURN_EX(LayoutCompatible(obj, incomingLayout), base::TempString("Object '{}' has incompatible source layout {}", *obj, incomingLayout));
			DEBUG_CHECK_RETURN_EX(LayoutCompatible(obj, outgoingLayout), base::TempString("Object '{}' has incompatible target layout {}", *obj, outgoingLayout));
			DEBUG_CHECK_RETURN(ensureResourceState(obj, incomingLayout, nullptr, outgoingLayout));
#endif

			auto op = allocCommand<OpResourceLayoutBarrier>();
			op->id = obj->id();
			op->sourceLayout = incomingLayout;
			op->targetLayout = outgoingLayout;
			op->firstMip = 0;
			op->numMips = 0;
			op->firstSlice = 0;
			op->numSlices = 0;
		}

		void CommandWriter::opTransitionImageRangeLayout(const ImageObject* obj, uint8_t firstMip, uint8_t numMips, ResourceLayout incomingLayout, ResourceLayout outgoingLayout)
		{
			DEBUG_CHECK_RETURN(obj != nullptr);
			DEBUG_CHECK_RETURN(incomingLayout != outgoingLayout);

#ifdef VALIDATE_RESOURCE_LAYOUTS
			DEBUG_CHECK_RETURN(LayoutCompatible(obj, incomingLayout));
			DEBUG_CHECK_RETURN(LayoutCompatible(obj, outgoingLayout));
			DEBUG_CHECK_RETURN(obj->subResourceLayouts());
#endif
			DEBUG_CHECK_RETURN(firstMip < obj->mips());
			DEBUG_CHECK_RETURN(firstMip + numMips <= obj->mips());

#ifdef VALIDATE_RESOURCE_LAYOUTS
			SubImageRegion region;
			region.firstMip = firstMip;
			region.numMips = numMips;
			region.firstSlice = 0;
			region.numSlices = obj->slices();
			DEBUG_CHECK_RETURN(ensureResourceState(obj, incomingLayout, &region, outgoingLayout));
#endif

			auto op = allocCommand<OpResourceLayoutBarrier>();
			op->id = obj->id();
			op->sourceLayout = incomingLayout;
			op->targetLayout = outgoingLayout;
			op->firstMip = firstMip;
			op->numMips = numMips;
			op->firstSlice = 0;
			op->numSlices = obj->slices();
		}

		void CommandWriter::opTransitionImageArrayRangeLayout(const ImageObject* obj, uint8_t firstMip, uint8_t numMips, uint32_t firstSlice, uint32_t numSlices, ResourceLayout incomingLayout, ResourceLayout outgoingLayout)
		{
			DEBUG_CHECK_RETURN(obj != nullptr);
			DEBUG_CHECK_RETURN(incomingLayout != outgoingLayout);
			DEBUG_CHECK_RETURN(obj->subResourceLayouts());
			DEBUG_CHECK_RETURN(firstMip < obj->mips());
			DEBUG_CHECK_RETURN(firstMip + numMips <= obj->mips());
			DEBUG_CHECK_RETURN(firstSlice < obj->slices());
			DEBUG_CHECK_RETURN(firstSlice + numSlices <= obj->slices());

#ifdef VALIDATE_RESOURCE_LAYOUTS
			DEBUG_CHECK_RETURN(LayoutCompatible(obj, incomingLayout));
			DEBUG_CHECK_RETURN(LayoutCompatible(obj, outgoingLayout));

			SubImageRegion region;
			region.firstMip = firstMip;
			region.numMips = numMips;
			region.firstSlice = firstSlice;
			region.numSlices = numSlices;
			DEBUG_CHECK_RETURN(ensureResourceState(obj, incomingLayout, &region, outgoingLayout));
#endif
			auto op = allocCommand<OpResourceLayoutBarrier>();
			op->id = obj->id();
			op->sourceLayout = incomingLayout;
			op->targetLayout = outgoingLayout;
			op->firstMip = firstMip;
			op->numMips = numMips;
			op->firstSlice = firstSlice;
			op->numSlices = numSlices;
		}

		void CommandWriter::opTransitionImageArrayRangeLayout(const ImageObject* obj, uint32_t firstSlice, uint32_t numSlices, ResourceLayout incomingLayout, ResourceLayout outgoingLayout)
		{
			DEBUG_CHECK_RETURN(obj != nullptr);
			DEBUG_CHECK_RETURN(incomingLayout != outgoingLayout);
			DEBUG_CHECK_RETURN(obj->subResourceLayouts());
			DEBUG_CHECK_RETURN(firstSlice < obj->slices());
			DEBUG_CHECK_RETURN(firstSlice + numSlices <= obj->slices());

#ifdef VALIDATE_RESOURCE_LAYOUTS
			DEBUG_CHECK_RETURN(LayoutCompatible(obj, incomingLayout));
			DEBUG_CHECK_RETURN(LayoutCompatible(obj, outgoingLayout));

			SubImageRegion region;
			region.firstMip = 0;
			region.numMips = obj->mips();
			region.firstSlice = firstSlice;
			region.numSlices = numSlices;
			DEBUG_CHECK_RETURN(ensureResourceState(obj, incomingLayout, &region, outgoingLayout));
#endif

			auto op = allocCommand<OpResourceLayoutBarrier>();
			op->id = obj->id();
			op->sourceLayout = incomingLayout;
			op->targetLayout = outgoingLayout;
			op->firstMip = 0;
			op->numMips = obj->mips();
			op->firstSlice = firstSlice;
			op->numSlices = numSlices;
		}

        //--

		void CommandWriter::linkUpdate(OpUpdate* op)
		{
			op->stagingBufferOffset = 0;
			op->next = nullptr;

			if (m_writeBuffer->m_gatheredState.dynamicBufferUpdatesHead == nullptr)
				m_writeBuffer->m_gatheredState.dynamicBufferUpdatesHead = op;
			else
				m_writeBuffer->m_gatheredState.dynamicBufferUpdatesTail->next = op;
			m_writeBuffer->m_gatheredState.dynamicBufferUpdatesTail = op;
		}

		static const uint32_t MAX_INLINED_BUFFER_PAYLOAD = 1024;
		static const uint32_t MAX_INLINED_IMAGE_PAYLOAD = 4096;

		void* CommandWriter::opUpdateDynamicPtr(const IDeviceObject* dynamicObject, const ResourceCopyRange& range)
		{
			DEBUG_CHECK_RETURN_V(dynamicObject, nullptr);

			if (const auto* dynamicBuffer = base::rtti_cast<BufferObject>(dynamicObject))
			{
				DEBUG_CHECK_RETURN_V(dynamicBuffer, nullptr);// , "Unable to update invalid buffer");
				DEBUG_CHECK_RETURN_V(dynamicBuffer->dynamic(), nullptr);// , "Dynamic update of buffer requires a buffer created with dynamic flag");
				DEBUG_CHECK_RETURN_V(range.buffer.size, nullptr);

				DEBUG_CHECK_RETURN_V(range.buffer.offset < dynamicBuffer->size(), nullptr);// , "Update offset is not within buffer bounds, something is really wrong");

				auto maxUpdateSize = dynamicBuffer->size() - range.buffer.offset;
				DEBUG_CHECK_RETURN_V(range.buffer.size <= maxUpdateSize, nullptr);// , "Dynamic data to update goes over the buffer range");


#ifdef VALIDATE_RESOURCE_LAYOUTS
				DEBUG_CHECK_RETURN_V(ensureResourceState(dynamicObject, ResourceLayout::CopyDest), nullptr);
#endif

				auto dataSize = std::min<uint32_t>(maxUpdateSize, range.buffer.size);
				auto maxLocalPayload = std::max<uint32_t>(MAX_INLINED_BUFFER_PAYLOAD, m_writeEndPtr - m_writePtr);

				OpUpdate* op;
				if (dataSize < maxLocalPayload)
				{
					op = allocCommand<OpUpdate>(dataSize);
					op->dataBlockPtr = op->payload();
				}
				else
				{
					op = allocCommand<OpUpdate>();
					op->dataBlockPtr = m_writeBuffer->m_pages->allocateOustandingBlock(dataSize, 16);
				}

				op->id = dynamicBuffer->id();
				op->dataBlockSize = dataSize;
				op->range = range;

				linkUpdate(op);

				return op->dataBlockPtr;
			}
			else if (const auto* dynamicImage = base::rtti_cast<ImageObject>(dynamicObject))
			{ 
				DEBUG_CHECK_RETURN_V(dynamicImage, nullptr);// .id(), "Unable to update invalid image");
				DEBUG_CHECK_RETURN_V(dynamicImage->dynamic(), nullptr);//, "Dynamic update of image requires an image created with dynamic flag");
				DEBUG_CHECK_RETURN_V(range.image.mip < dynamicImage->mips(), nullptr);
				DEBUG_CHECK_RETURN_V(range.image.slice< dynamicImage->slices(), nullptr);

				const auto mipWidth = std::max<uint32_t>(dynamicImage->width() >> range.image.mip, 1);
				const auto mipHeight = std::max<uint32_t>(dynamicImage->height() >> range.image.mip, 1);
				const auto mipDepth = std::max<uint32_t>(dynamicImage->depth() >> range.image.mip, 1);
				DEBUG_CHECK_RETURN_V(range.image.offsetX < mipWidth && range.image.offsetY < mipHeight && range.image.offsetZ < mipDepth, nullptr);

				auto maxAllowedWidth = mipWidth - range.image.offsetX;
				auto maxAllowedHeight = mipHeight - range.image.offsetY;
				auto maxAllowedDepth = mipDepth - range.image.offsetZ;
				DEBUG_CHECK_RETURN_V(range.image.sizeX <= maxAllowedWidth && range.image.sizeY <= maxAllowedHeight && range.image.sizeZ <= maxAllowedDepth, nullptr);

				const auto dataSize = CalcUpdateMemorySize(dynamicImage->format(), range);
				const auto maxLocalPayload = std::max<uint32_t>(MAX_INLINED_IMAGE_PAYLOAD, m_writeEndPtr - m_writePtr);

#ifdef VALIDATE_RESOURCE_LAYOUTS
				{
					SubImageRegion region;
					region.firstMip = range.image.mip;
					region.firstSlice = range.image.slice;
					region.numMips = 1;
					region.numSlices = 1;
					DEBUG_CHECK_RETURN_V(ensureResourceState(dynamicObject, ResourceLayout::CopyDest, &region), nullptr);
				}
#endif

				OpUpdate* op;
				if (dataSize < maxLocalPayload)
				{
					op = allocCommand<OpUpdate>(dataSize);
					op->dataBlockPtr = op->payload();
				}
				else
				{
					op = allocCommand<OpUpdate>();
					op->dataBlockPtr = m_writeBuffer->m_pages->allocateOustandingBlock(dataSize, 16);
				}

				op->id = dynamicImage->id();
				op->dataBlockSize = dataSize;
				op->range = range;

				linkUpdate(op);

				return op->dataBlockPtr;
			}

			return nullptr;
		}

        void CommandWriter::opUpdateDynamicImage(const ImageObject* dynamicImage, const base::image::ImageView& sourceData, uint8_t mipIndex /*= 0*/, uint32_t sliceIndex /*= 0*/, uint32_t offsetX /*= 0*/, uint32_t offsetY /*= 0*/, uint32_t offsetZ /*= 0*/)
        {
            DEBUG_CHECK_RETURN(!sourceData.empty());//, "Empty data for update");

			ResourceCopyRange range;
			range.image.mip = mipIndex;
			range.image.slice = sliceIndex;
			range.image.offsetX = offsetX;
			range.image.offsetY = offsetY;
			range.image.offsetZ = offsetZ;
			range.image.sizeX = sourceData.width();
			range.image.sizeY = sourceData.height();
			range.image.sizeZ = sourceData.depth();

			void* targetUpdatePtr = opUpdateDynamicPtr(dynamicImage, range);
			DEBUG_CHECK_RETURN(targetUpdatePtr);
			
			// copy image data into inlined storage
			// NOTE: we use the image utilities since the image view may be a sub-rect of bigger view
			{
				PC_SCOPE_LVL2(InternalImageCopy);

				// NOTE: we assume that pixels in the payload buffer are packed tightly (hence the native layout)
				auto targetRect = base::image::ImageView(base::image::NATIVE_LAYOUT, sourceData.format(), sourceData.channels(), targetUpdatePtr, sourceData.width(), sourceData.height(), sourceData.depth());
				base::image::Copy(sourceData, targetRect);
			}
        }

		void CommandWriter::opUpdateDynamicBuffer(const BufferObject* dynamicBuffer, uint32_t dataOffset, uint32_t dataSize, const void* dataPtr)
		{
			ResourceCopyRange range;
			range.buffer.offset = dataOffset;
			range.buffer.size = dataSize;

			void* targetUpdatePtr = opUpdateDynamicPtr(dynamicBuffer, range);
			DEBUG_CHECK_RETURN(targetUpdatePtr);

			memcpy(targetUpdatePtr, dataPtr, dataSize);
		}

        //---

		static bool IsCopiableObject(const IDeviceObject* src, const ResourceCopyRange& range, uint32_t& outDataSize)
		{
			if (const auto* srcImage = base::rtti_cast<ImageObject>(src))
			{
				DEBUG_CHECK_RETURN_V(srcImage->copyCapable(), false);
				DEBUG_CHECK_RETURN_V(range.image.mip < srcImage->mips(), false);
				DEBUG_CHECK_RETURN_V(range.image.slice< srcImage->slices(), false);

				const auto mipWidth = std::max<uint32_t>(srcImage->width() >> range.image.mip, 1);
				const auto mipHeight = std::max<uint32_t>(srcImage->height() >> range.image.mip, 1);
				const auto mipDepth = std::max<uint32_t>(srcImage->depth() >> range.image.mip, 1);

				DEBUG_CHECK_RETURN_V(range.image.offsetX < mipWidth, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetY < mipHeight, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetZ < mipDepth, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetX + range.image.sizeX <= mipWidth, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetY + range.image.sizeY <= mipHeight, false);
				DEBUG_CHECK_RETURN_V(range.image.offsetZ + range.image.sizeZ <= mipDepth, false);

				outDataSize = CalcUpdateMemorySize(srcImage->format(), range);
				return true;
			}
			else if (const auto* srcBuffer = base::rtti_cast<BufferObject>(src))
			{
				DEBUG_CHECK_RETURN_V(srcBuffer->copyCapable(), false);
				DEBUG_CHECK_RETURN_V(range.buffer.offset < srcBuffer->size(), false);
				DEBUG_CHECK_RETURN_V(range.buffer.offset + range.buffer.size <= srcBuffer->size(), false);

				outDataSize = range.buffer.size;
				return true;
			}

			return false;
		}

		static bool CheckSizeRanges(const IDeviceObject* src, const ResourceCopyRange& srcRange, const ResourceCopyRange& destRange)
		{
			if (src->cls()->is<BufferObject>())
			{
				DEBUG_CHECK_RETURN_V(srcRange.buffer.size == destRange.buffer.size, false);
			}
			else if (src->cls()->is<ImageObject>())
			{
				//DEBUG_CHECK_RETURN_V(srcRange.image.mip == 1, false);
				//DEBUG_CHECK_RETURN_V(srcRange.image.numSlices == 1, false);
				//DEBUG_CHECK_RETURN_V(destRange.image.numMips == 1, false);
				//DEBUG_CHECK_RETURN_V(destRange.image.numSlices == 1, false);
			}

			return true;
		}

		static bool CheckOverlapRanges(const IDeviceObject* src, const ResourceCopyRange& srcRange, const ResourceCopyRange& destRange)
		{
			if (src->cls()->is<BufferObject>())
			{
				const auto srcEnd = srcRange.buffer.offset + srcRange.buffer.size;
				const auto destEnd = destRange.buffer.offset + destRange.buffer.size;
				DEBUG_CHECK_RETURN_V(srcRange.buffer.offset >= destEnd || srcEnd <= destRange.buffer.offset, false);
			}
			else if (src->cls()->is<ImageObject>())
			{
				if (srcRange.image.mip == destRange.image.mip && srcRange.image.slice == destRange.image.slice)
				{
					const auto srcEndX = srcRange.image.offsetX + srcRange.image.sizeX;
					const auto srcEndY = srcRange.image.offsetY + srcRange.image.sizeY;
					const auto srcEndZ = srcRange.image.offsetZ + srcRange.image.sizeZ;
					const auto destEndX = destRange.image.offsetX + destRange.image.sizeX;
					const auto destEndY = destRange.image.offsetY + destRange.image.sizeY;
					const auto destEndZ = destRange.image.offsetZ + destRange.image.sizeZ;

					bool separated = false;
					separated |= (destRange.image.offsetX >= srcEndX) || (srcRange.image.offsetX >= destEndX);
					separated |= (destRange.image.offsetY >= srcEndY) || (srcRange.image.offsetY >= destEndY);
					separated |= (destRange.image.offsetZ >= srcEndZ) || (srcRange.image.offsetZ >= destEndZ);
					DEBUG_CHECK_RETURN_V(separated, false);
				}
			}

			return true;
		}

        void CommandWriter::opCopy(const IDeviceObject* src, const ResourceCopyRange& srcRange, const IDeviceObject* dest, const ResourceCopyRange& destRange)
        {
            DEBUG_CHECK_RETURN(src);//, "Invalid source buffer");
            DEBUG_CHECK_RETURN(dest);//, "Invalid target buffer");
			//DEBUG_CHECK_RETURN(src != dest);

			uint32_t srcDataSize = 0, destDataSize = 0;
			DEBUG_CHECK_RETURN(IsCopiableObject(src, srcRange, srcDataSize));
			DEBUG_CHECK_RETURN(IsCopiableObject(dest, destRange, destDataSize));
			DEBUG_CHECK_RETURN(srcDataSize == destDataSize);            

			DEBUG_CHECK_RETURN(CheckSizeRanges(src, srcRange, destRange));

			if (src->id() == dest->id())
			{
				DEBUG_CHECK_RETURN(CheckOverlapRanges(src, srcRange, destRange));
			}

#ifdef VALIDATE_RESOURCE_LAYOUTS
			if (src->cls()->is<ImageObject>())
			{
				SubImageRegion region;
				region.firstMip = srcRange.image.mip;
				region.firstSlice = srcRange.image.slice;
				region.numMips = 1;
				region.numSlices = 1;
				DEBUG_CHECK_RETURN(ensureResourceState(src, ResourceLayout::CopySource, &region));
			}
			else
			{
				DEBUG_CHECK_RETURN(ensureResourceState(src, ResourceLayout::CopySource));
			}

			if (src->cls()->is<ImageObject>())
			{
				SubImageRegion region;
				region.firstMip = srcRange.image.mip;
				region.firstSlice = srcRange.image.slice;
				region.numMips = 1;
				region.numSlices = 1;
				DEBUG_CHECK_RETURN(ensureResourceState(dest, ResourceLayout::CopyDest, &region));
			}
			else
			{
				DEBUG_CHECK_RETURN(ensureResourceState(dest, ResourceLayout::CopyDest));
			}
#endif

			auto* op = allocCommand<OpCopy>();
			op->src = src->id();
			op->srcRange = srcRange;
			op->dest = dest->id();
			op->destRange = destRange;
        }

		void CommandWriter::opCopyBuffer(const BufferObject* src, uint32_t srcOffset, const BufferObject* dest, uint32_t destOffset, uint32_t size)
		{
			ResourceCopyRange srcRange;
			srcRange.buffer.offset = srcOffset;
			srcRange.buffer.size = size;

			ResourceCopyRange destRange;
			destRange.buffer.offset = destOffset;
			destRange.buffer.size = size;

			opCopy(src, srcRange, dest, destRange);
		}

		void CommandWriter::opCopyBufferToImage(const BufferObject* src, uint32_t srcOffset, const ImageObject* dest, uint32_t offsetX, uint32_t offsetY, uint32_t offsetZ, uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, uint8_t mipIndex /*= 0*/, uint32_t sliceIndex /*= 0*/)
		{
			ResourceCopyRange destRange;
			destRange.image.offsetX = offsetX;
			destRange.image.offsetY = offsetY;
			destRange.image.offsetZ = offsetZ;
			destRange.image.mip = mipIndex;
			destRange.image.slice = sliceIndex;
			destRange.image.sizeX = sizeX;
			destRange.image.sizeY = sizeY;
			destRange.image.sizeZ = sizeZ;

			const auto imageDataSize = CalcUpdateMemorySize(dest->format(), destRange);
			DEBUG_CHECK_RETURN(imageDataSize != 0);

			ResourceCopyRange srcRange;
			srcRange.buffer.offset = srcOffset;
			srcRange.buffer.size = imageDataSize;

			opCopy(src, srcRange, dest, destRange);
		}

		void CommandWriter::opCopyImageToBuffer(const ImageObject* src, uint32_t offsetX, uint32_t offsetY, uint32_t offsetZ, uint32_t sizeX, uint32_t sizeY, uint32_t sizeZ, const BufferObject* dest, uint32_t destOffset, uint8_t mipIndex /*= 0*/, uint32_t sliceIndex /*= 0*/)
		{
			ResourceCopyRange srcRange;
			srcRange.image.offsetX = offsetX;
			srcRange.image.offsetY = offsetY;
			srcRange.image.offsetZ = offsetZ;
			srcRange.image.mip = mipIndex;
			srcRange.image.slice = sliceIndex;
			srcRange.image.sizeX = sizeX;
			srcRange.image.sizeY = sizeY;
			srcRange.image.sizeZ = sizeZ;

			const auto imageDataSize = CalcUpdateMemorySize(src->format(), srcRange);
			DEBUG_CHECK_RETURN(imageDataSize != 0);

			ResourceCopyRange destRange;
			destRange.buffer.offset = destOffset;
			destRange.buffer.size = imageDataSize;

			opCopy(src, srcRange, dest, destRange);
		}

        //---

		void CommandWriter::opDownloadData(const IDeviceObject* obj, const ResourceCopyRange& range, IDownloadAreaObject* area, uint32_t areaOffset, IDownloadDataSink* sink)
		{
			DEBUG_CHECK_RETURN(obj);
			DEBUG_CHECK_RETURN(sink);
			DEBUG_CHECK_RETURN(area);

			if (const auto* image = base::rtti_cast<ImageObject>(obj))
			{
				DEBUG_CHECK_RETURN(range.image.mip < image->mips());
				DEBUG_CHECK_RETURN(range.image.slice < image->slices());

				const auto mipWidth = std::max<uint32_t>(1, image->width() >> range.image.mip);
				const auto mipHeight = std::max<uint32_t>(1, image->height() >> range.image.mip);
				const auto mipDepth = std::max<uint32_t>(1, image->depth() >> range.image.mip);

				DEBUG_CHECK_RETURN_EX(range.image.offsetX < mipWidth, "Offset lies beyond image bounds");
				DEBUG_CHECK_RETURN_EX(range.image.offsetY < mipHeight, "Offset lies beyond image bounds");
				DEBUG_CHECK_RETURN_EX(range.image.offsetZ < mipDepth, "Offset lies beyond image bounds");
				DEBUG_CHECK_RETURN_EX(range.image.offsetX + range.image.sizeX <= mipWidth, "Size of area to download is outside image bounds");
				DEBUG_CHECK_RETURN_EX(range.image.offsetY + range.image.sizeY <= mipHeight, "Size of area to download is outside image bounds");
				DEBUG_CHECK_RETURN_EX(range.image.offsetZ + range.image.sizeZ <= mipDepth, "Size of area to download is outside image bounds");

				const auto& formatInfo = GetImageFormatInfo(image->format());
				if (formatInfo.compressed)
				{
					DEBUG_CHECK_RETURN_EX((range.image.offsetX & 3) == 0, "Compressed images require offset and size aligned to block size (4)");
					DEBUG_CHECK_RETURN_EX((range.image.offsetY & 3) == 0, "Compressed images require offset and size aligned to block size (4)");
					DEBUG_CHECK_RETURN_EX((range.image.offsetZ & 3) == 0, "Compressed images require offset and size aligned to block size (4)");
					DEBUG_CHECK_RETURN_EX((range.image.sizeX & 3) == 0, "Compressed images require offset and size aligned to block size (4)");
					DEBUG_CHECK_RETURN_EX((range.image.sizeY & 3) == 0, "Compressed images require offset and size aligned to block size (4)");
					DEBUG_CHECK_RETURN_EX((range.image.sizeZ & 3) == 0, "Compressed images require offset and size aligned to block size (4)");
				}

				const auto dataSize = (range.image.sizeX * range.image.sizeY * range.image.sizeZ) * formatInfo.bitsPerPixel / 8;

#ifdef VALIDATE_RESOURCE_LAYOUTS
				{
					SubImageRegion region;
					region.firstMip = range.image.mip;
					region.firstSlice = range.image.slice;
					region.numMips = 1;
					region.numSlices = 1;
					DEBUG_CHECK_RETURN(ensureResourceState(image, ResourceLayout::CopySource, &region));
				}
#endif

				auto op = allocCommand<OpDownload>();
				op->id = obj->id();
				op->areaId = area->id();
				op->range = range;
				op->offsetInArea = areaOffset;
				op->sizeInArea = dataSize;
				op->sink = sink;
				op->area = area;

				m_writeBuffer->m_downloadSinks.pushBack(AddRef(sink));
				m_writeBuffer->m_downloadAreas.pushBack(AddRef(area));
			}
			else if (const auto* buffer = base::rtti_cast<BufferObject>(obj))
			{
				DEBUG_CHECK_RETURN(range.buffer.offset < buffer->size());
				DEBUG_CHECK_RETURN(range.buffer.offset + range.buffer.size <= buffer->size());

#ifdef VALIDATE_RESOURCE_LAYOUTS
				DEBUG_CHECK_RETURN(ensureResourceState(buffer, ResourceLayout::CopySource));
#endif

				auto op = allocCommand<OpDownload>();
				op->id = obj->id();
				op->areaId = area->id();
				op->offsetInArea = areaOffset;
				op->sizeInArea = range.buffer.size;
				op->range = range;
				op->sink = sink;

				m_writeBuffer->m_downloadSinks.pushBack(AddRef(sink));
				m_writeBuffer->m_downloadAreas.pushBack(AddRef(area));
			}
		}

        //---

        void CommandWriter::opBindDescriptorEntries(base::StringID binding, const DescriptorEntry* entries, uint32_t count)
        {
            DEBUG_CHECK_RETURN(binding);

			const DescriptorInfo* layout = nullptr;
			const auto id = DescriptorID::FromDescriptor(entries, count, &layout);
            DEBUG_CHECK_RETURN(id);

            auto* data = uploadDescriptor(id, layout, entries, count);
            DEBUG_CHECK_RETURN(data);

            auto op = allocCommand<OpBindDescriptor>();
			op->layout = layout;
            op->binding = binding;
            op->data = data;

            m_currentParameterBindings[binding] = id;

#ifdef VALIDATE_DESCRIPTOR_BOUND_RESOURCES
			{
				m_currentParameterData[binding] = data;

				for (uint32_t i = 0; i < count; ++i)
					if (entries[i].viewPtr && m_trackedObjectViews.contains(entries[i].viewPtr))
						m_trackedObjectViews.insert(AddRef(entries[i].viewPtr));
			}
#endif
        }

        void* CommandWriter::allocConstants(uint32_t size, const command::OpUploadConstants*& outOffsetPtr)
        {
            ASSERT_EX(size > 0, "Recording contants with zero size is not a good idea");

            // allocate space
            //auto uploadSpaceOffset = base::Align<uint32_t>(m_writeBuffer->m_gatheredState.totalConstantsUploadSize, 256);
            //m_writeBuffer->m_gatheredState.totalConstantsUploadSize = uploadSpaceOffset + base::Align<uint32_t>(size, 16);

            // align size to the vector size
            auto alignedSize = base::Align<uint32_t>(size, 16);
            auto maxLocalPayload = std::max<uint32_t>(1024, m_writeEndPtr - m_writePtr);

            // allocate op and copy the data
            OpUploadConstants* op = nullptr;
            if (alignedSize < maxLocalPayload)
            {
                op = allocCommand<OpUploadConstants>(alignedSize);
                op->dataPtr = op->payload();
            }
            else
            {
                op = allocCommand<OpUploadConstants>();
                op->dataPtr = m_writeBuffer->m_pages->allocateOustandingBlock(alignedSize, 16);
            }

            op->dataSize = alignedSize;
            op->nextConstants = nullptr;

            // link in the list
            if (m_writeBuffer->m_gatheredState.constantUploadTail)
                m_writeBuffer->m_gatheredState.constantUploadTail->nextConstants = op;
            else
                m_writeBuffer->m_gatheredState.constantUploadHead = op;
            m_writeBuffer->m_gatheredState.constantUploadTail = op;
            outOffsetPtr = op;

            return op->dataPtr;
        }

		DescriptorEntry* CommandWriter::uploadDescriptor(DescriptorID layoutID, const DescriptorInfo* layout, const DescriptorEntry* entries, uint32_t count)
        {
            DEBUG_CHECK_RETURN_V(!layoutID.empty(), nullptr);
            DEBUG_CHECK_RETURN_V(entries != nullptr, nullptr);
            DEBUG_CHECK_RETURN_V(count != 0, nullptr);
            DEBUG_CHECK_RETURN_V(count == layout->size(), nullptr);

            const auto additionalMemory = count * sizeof(DescriptorEntry);

            auto op  = allocCommand<OpUploadDescriptor>(additionalMemory);
            op->layout = layout;
            op->nextParameters = nullptr;
            memcpy(op->payload(), entries, additionalMemory);

            // link in the list
            auto entryList  = &m_writeBuffer->m_gatheredState.parameterUploadEntries[layoutID.value()];
            if (entryList->head == nullptr)
            {
                ASSERT(entryList->tail == nullptr);
                ASSERT(entryList->nextIndex == 0);

                // new type of parameters used
                entryList->head = op;
                entryList->tail = op;

                // link the entry in the list of active layouts
                // NOTE: order does not matter here
                entryList->next = m_writeBuffer->m_gatheredState.parameterUploadActiveList;
                m_writeBuffer->m_gatheredState.parameterUploadActiveList = entryList;
            }
            else
            {
                ASSERT(entryList->tail != nullptr);
                ASSERT(entryList->nextIndex != 0);

                // link to existing elements
                entryList->tail->nextParameters = op;
                entryList->tail = op;
            }

            // assign an ID to the parameter set
            op->index = entryList->nextIndex;
            entryList->nextIndex += 1;

            // upload specified constant data
            for (uint32_t i = 0; i < count; ++i)
            {
                auto& entry = ((DescriptorEntry*)op->payload())[i];

                if (entry.type == DeviceObjectViewType::ConstantBuffer && entry.inlinedConstants.sourceDataPtr)
                {
                    DEBUG_CHECK(!entry.id);

					const command::OpUploadConstants* uploadCommand = nullptr;
                    void* targetData = allocConstants(entry.size, uploadCommand);
                    memcpy(targetData, entry.inlinedConstants.sourceDataPtr, entry.size);

					entry.inlinedConstants.uploadedDataPtr = uploadCommand;
                }
            }

            // return the view on the data
            return op->payload<DescriptorEntry>();
        }

        //---

    } // command
}  // rendering
