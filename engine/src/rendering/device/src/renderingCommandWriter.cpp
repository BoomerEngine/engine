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
#include "renderingParametersLayoutID.h"
#include "renderingParametersLayoutInfo.h"
#include "renderingShaderLibrary.h"
#include "renderingBufferView.h"
#include "renderingOutput.h"
#include "renderingDeviceService.h"
#include "renderingDeviceApi.h"

#include "base/containers/include/bitUtils.h"
#include "base/image/include/imageVIew.h"
#include "base/image/include/imageUtils.h"
#include "base/system/include/thread.h"
#include "base/memory/include/pageCollection.h"

namespace rendering
{
    namespace command
    {

//#define ALWAYS_ALLOC_TO_EXTERNAL_MEMORY

        //--

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
                }
                else
                {
                    m_writeBuffer->m_activeParameterBindings = std::move(m_currentParameterBindings);
                }

                m_writePtr = nullptr;
                m_writeEndPtr = nullptr;
                m_writeBuffer = nullptr;
                m_lastCommand = nullptr;

                m_currentPass = nullptr;
                m_numOpenedBlocks = 0;
                m_currentPassRts = 0;
                m_currentPassViewports = 0;
                m_currentIndexBufferElementCount = 0;
                m_currentVertexBufferRemainingSize.reset();
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
            if (output->prepare(&ret.color, &ret.depth, ret.size))
            {
                auto op = allocCommand<OpAcquireOutput>();
                op->output = output->id();
                op->size = ret.size;
                op->colorView = ret.color;
                op->depthView = ret.depth;
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

        void CommandWriter::opBeingPass(const FrameBuffer& frameBuffer, uint8_t numViewports, const FrameBufferViewportState* intialViewportSettings)
        {
            DEBUG_CHECK_EX(!m_currentPass, "Recursive passes are not allowed");
            DEBUG_CHECK_EX(numViewports >= 1 && numViewports <= 16, "Invalid number of viewports");

            numViewports = std::clamp<uint8_t>(numViewports, 1, 16);

            auto frameBufferValid = frameBuffer.validate();
            DEBUG_CHECK_EX(frameBufferValid, "Cannot enter a pass with invalid frame buffer");
            if (frameBufferValid)
            {
                auto payloadSize = intialViewportSettings ? (sizeof(FrameBufferViewportState) * numViewports) : 0;

                auto op = allocCommand<OpBeginPass>(payloadSize);
                op->frameBuffer = frameBuffer;
                op->hasBarriers = false;
                op->numViewports = numViewports;

                if (intialViewportSettings)
                {
                    op->hasInitialViewportSetup = true;
                    memcpy(op->payload(), intialViewportSettings, payloadSize);
                }
                else
                {
                    op->hasInitialViewportSetup = false;
                }

                m_currentPass = op;
                m_currentPassViewports = numViewports;
                m_currentPassRts = frameBuffer.validColorSurfaces();
            }
        }

        void CommandWriter::opEndPass()
        {
            DEBUG_CHECK_EX(m_currentPass, "Not in pass");
            if (m_currentPass)
            {
                DEBUG_CHECK_EX(!m_isChildBufferWithParentPass, "Cannot end pass that started in parent command buffer");
                if (!m_isChildBufferWithParentPass)
                {
                    auto op = allocCommand<OpEndPass>();
                    m_currentPass = nullptr;
                }
            }
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
            DEBUG_CHECK(m_numOpenedBlocks > 0);

            if (m_numOpenedBlocks > 0)
            {
                auto op = allocCommand<OpEndBlock>();
                m_numOpenedBlocks -= 1;
            }
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
            DEBUG_CHECK_EX(m_currentPass == nullptr, "External command buffers can only be attached outside the pass");

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

#define DEBUG_CHECK_PASS_ONLY() \
        DEBUG_CHECK_EX(m_currentPass != nullptr, "No active render pass"); \
        if (m_currentPass == nullptr) return;

        void CommandWriter::opSetViewportRect(uint8_t viewport, const base::Rect& viewportRect)
        {
            DEBUG_CHECK_PASS_ONLY();
            DEBUG_CHECK_EX(viewport < m_currentPassViewports, "Invalid viewport index, current pass defines less viewports");
            DEBUG_CHECK_EX(viewportRect.width() >= 0, "Width can't be negataive");
            DEBUG_CHECK_EX(viewportRect.height() >= 0, "Height can't be negataive");

            if (viewport < m_currentPassViewports)
            {
                auto op = allocCommand<OpSetViewportRect>();
                op->viewportIndex = viewport;
                op->rect = viewportRect;
            }
        }

        void CommandWriter::opSetViewportRect(uint8_t viewport, int x, int y, int w, int h)
        {
            opSetViewportRect(viewport, base::Rect(x, y, x + w, y + h));
        }

        void CommandWriter::opSetViewportDepthRange(uint8_t viewport, float minZ, float maxZ)
        {
            DEBUG_CHECK_PASS_ONLY();
            DEBUG_CHECK_EX(viewport < m_currentPassViewports, "Invalid viewport index, current pass defines less viewports");

            if (viewport < m_currentPassViewports)
            {
                auto op = allocCommand<OpSetViewportDepthRange>();
                op->viewportIndex = viewport;
                op->minZ = minZ;
                op->maxZ = maxZ;
            }
        }

        void CommandWriter::opSetColorMask(uint8_t rtIndex /*= 0*/, uint8_t mask /*= 0xF*/)
        {
            DEBUG_CHECK_PASS_ONLY();
            DEBUG_CHECK_EX(rtIndex < m_currentPassRts, "Invalid color render target index, current pass defines less color render targets");

            if (rtIndex < m_currentPassRts)
            {
                auto op = allocCommand<OpSetColorMask>();
                op->rtIndex = rtIndex;
                op->colorMask = mask;
            }
        }

        void CommandWriter::opSetBlendState(uint8_t renderTargetIndex, const BlendState& state)
        {
            DEBUG_CHECK_PASS_ONLY();
            DEBUG_CHECK_EX(renderTargetIndex < m_currentPassRts, "Invalid color render target index, current pass defines less color render targets");

            if (renderTargetIndex < m_currentPassRts)
            {
                auto op = allocCommand<OpSetBlendState>();
                op->rtIndex = renderTargetIndex;
                op->state = state;
            }
        }

        void CommandWriter::opSetBlendState(uint8_t renderTargetIndex)
        {
            BlendState state;
            opSetBlendState(renderTargetIndex, state);
        }

        void CommandWriter::opSetBlendState(uint8_t renderTargetIndex, BlendFactor src, BlendFactor dest)
        {
            BlendState state;
            state.srcAlphaBlendFactor = state.srcColorBlendFactor = src;
            state.destAlphaBlendFactor = state.destColorBlendFactor = dest;
            opSetBlendState(renderTargetIndex, state);
        }

        void CommandWriter::opSetBlendState(uint8_t renderTargetIndex, BlendOp op, BlendFactor src, BlendFactor dest)
        {
            BlendState state;
            state.srcAlphaBlendFactor = state.srcColorBlendFactor = src;
            state.destAlphaBlendFactor = state.destColorBlendFactor = dest;
            state.colorBlendOp = state.alphaBlendOp = op;
            opSetBlendState(renderTargetIndex, state);
        }

        void CommandWriter::opSetBlendState(uint8_t renderTargetIndex, BlendFactor srcColor, BlendFactor destColor, BlendFactor srcAlpha, BlendFactor destAlpha)
        {
            BlendState state;
            state.srcAlphaBlendFactor = srcAlpha;
            state.srcColorBlendFactor = srcColor;
            state.destAlphaBlendFactor = destAlpha;
            state.destColorBlendFactor = destColor;
            opSetBlendState(renderTargetIndex, state);
        }

        void CommandWriter::opSetCullState(CullMode mode /*= CullMode::Back*/, FrontFace face /*= FrontFace::CW*/)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetCullState>();
            op->state.face = face;
            op->state.mode = mode;
        }

        void CommandWriter::opSetCullState(const CullState& state)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetCullState>();
            op->state = state;
        }

        void CommandWriter::opSetFillState(const FillState& state)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetFillState>();
            op->state = state;
        }

        void CommandWriter::opSetFillState(PolygonMode mode /*= PolygonMode::Fill*/, float lineWidth /*= 1.0f*/)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetFillState>();
            op->state.lineWidth = lineWidth;
            op->state.mode = mode;
        }

        void CommandWriter::opSetScissorState(bool enabled)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetScissorState>();
            op->state = enabled;
        }

        void CommandWriter::opSetScissorRect(uint8_t viewportIndex, const base::Rect& scissorRect)
        {
            DEBUG_CHECK_PASS_ONLY();
            DEBUG_CHECK_EX(viewportIndex < m_currentPassViewports, "Invalid viewport index, current pass defines less viewports");
            DEBUG_CHECK_EX(scissorRect.width() >= 0, "Width can't be negataive");
            DEBUG_CHECK_EX(scissorRect.height() >= 0, "Height can't be negataive");

            if (viewportIndex < m_currentPassViewports)
            {
                auto op = allocCommand<OpSetScissorRect>();
                op->viewportIndex = viewportIndex;
                op->rect = scissorRect;
            }
        }

        void CommandWriter::opSetScissorRect(uint8_t viewportIndex, int x, int y, int w, int h)
        {
            opSetScissorRect(viewportIndex, base::Rect(x, y, x + w, y + h));
        }

        void CommandWriter::opSetScissorBounds(uint8_t viewportIndex, int x0, int y0, int x1, int y1)
        {
            opSetScissorRect(viewportIndex, base::Rect(x0, y0, x1, y1));
        }

        void CommandWriter::opSetStencilState(const StencilState& state)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetStencilState>();
            op->state = state;
        }

        void CommandWriter::opSetStencilState(const StencilSideState& commonFaceState)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetStencilState>();
            op->state.enabled = 1;
            op->state.front = commonFaceState;
            op->state.back = commonFaceState;
        }

        void CommandWriter::opSetStencilState()
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetStencilState>();
            op->state.enabled = 0;
            op->state.front = StencilSideState();
            op->state.back = StencilSideState();
        }

        void CommandWriter::opSetStencilState(CompareOp compareOp, StencilOp failOp, StencilOp depthFailOp, StencilOp passOp, uint8_t reference /*= 0*/, uint8_t compareMask /*= 0xFF*/, uint8_t writeMask /*= 0xFF*/)
        {
            DEBUG_CHECK_PASS_ONLY();

            StencilSideState state;
            state.compareOp = compareOp;
            state.failOp = failOp;
            state.depthFailOp = depthFailOp;
            state.passOp = passOp;
            state.referenceValue = reference;
            state.writeMask = writeMask;
            state.compareMask = compareMask;

            auto op = allocCommand<OpSetStencilState>();
            op->state.enabled = 1;
            op->state.front = state;
            op->state.back = state;
        }

        void CommandWriter::opSetStencilState(const StencilSideState& frontState, const StencilSideState& backState)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetStencilState>();
            op->state.enabled = 1;
            op->state.front = frontState;
            op->state.back = backState;
        }

        void CommandWriter::opSetStencilReferenceValue(uint8_t value)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op  = allocCommand<OpSetStencilReference>();
            op->front = value;
            op->back = value;
        }

        void CommandWriter::opSetStencilReferenceValue(uint8_t frontValue, uint8_t backValue)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetStencilReference>();
            op->front = frontValue;
            op->back = backValue;
        }

        void CommandWriter::opSetStencilCompareMask(uint8_t mask)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op  = allocCommand<OpSetStencilCompareMask>();
            op->front = mask;
            op->back = mask;
        }

        void CommandWriter::opSetStencilCompareMask(uint8_t frontValue, uint8_t backValue)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetStencilCompareMask>();
            op->front = frontValue;
            op->back = backValue;
        }

        void CommandWriter::opSetStencilWriteMask(uint8_t mask)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op  = allocCommand<OpSetStencilWriteMask>();
            op->front = mask;
            op->back = mask;
        }

        void CommandWriter::opSetStencilWriteMask(uint8_t front, uint8_t back)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetStencilWriteMask>();
            op->front = front;
            op->back = back;
        }

        void CommandWriter::opSetDepthState(bool enable /*= true*/, bool write /*= true*/, CompareOp func /*= CompareOp::LessEqual*/)
        {
            DEBUG_CHECK_PASS_ONLY();

            DepthState state;
            state.enabled = enable;
            state.writeEnabled = write;
            state.depthCompareOp = func;

            auto op = allocCommand<OpSetDepthState>();
            op->state = state;
        }

        void CommandWriter::opSetDepthState(const DepthState& state)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op  = allocCommand<OpSetDepthState>();
            op->state = state;
        }

        void CommandWriter::opSetDepthClip(const DepthClipState& state)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetDepthClipState>();
            op->state = state;
        }

        void CommandWriter::opSetDepthClip(bool enabled, float min, float max)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetDepthClipState>();
            op->state.enabled = enabled;
            op->state.clipMin = min;
            op->state.clipMax = max;
        }

        void CommandWriter::opSetDepthBias(const DepthBiasState& state)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetDepthBiasState>();
            op->state = state;
        }

        void CommandWriter::opSetDepthBias(float constant, float slope, float clampValue)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op  = allocCommand<OpSetDepthBiasState>();
            op->state.enabled = 1;
            op->state.constant = constant;
            op->state.slope = slope;
            op->state.clamp = clampValue;
        }

        void CommandWriter::opSetPrimitiveState(const PrimitiveAssemblyState& state)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetPrimitiveAssemblyState>();
            op->state = state;
        }

        void CommandWriter::opSetPrimitiveType(PrimitiveTopology topology, bool enableVertexRestart /*= false*/)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetPrimitiveAssemblyState>();
            op->state.topology = topology;
            op->state.restartEnabled = enableVertexRestart ? 1 : 0;
        }

        void CommandWriter::opSetMultisampleState(const MultisampleState& state)
        {
            DEBUG_CHECK_PASS_ONLY();

            auto op = allocCommand<OpSetMultisampleState>();
            op->state = state;
        }

        //--

        void CommandWriter::opClearPassColor(uint32_t index, const base::Vector4& color)
        {
            DEBUG_CHECK_PASS_ONLY();

            DEBUG_CHECK_EX(index < m_currentPassRts, "Invalid render target index");
            if (index < m_currentPassRts)
            {
                DEBUG_CHECK_EX(m_currentPass->frameBuffer.color[index], "Clearing color render target that was never bound");
                if (m_currentPass->frameBuffer.color[index])
                {
                    auto op = allocCommand<OpClearPassColor>();
                    op->index = index;
                    op->color[0] = color.x;
                    op->color[1] = color.y;
                    op->color[2] = color.z;
                    op->color[3] = color.w;
                }
            }
        }

        void CommandWriter::opClearPassDepthStencil(float depth, uint8_t stencil, bool doClearDepth /*= true*/, bool doClearStencil /*= true*/)
        {
            DEBUG_CHECK_PASS_ONLY();

            DEBUG_CHECK_EX(m_currentPass->frameBuffer.depth, "Clearing color render target that was never bound");
            if (m_currentPass->frameBuffer.depth)
            {
                if (doClearStencil || doClearDepth)
                {
                    DEBUG_CHECK_EX(m_currentPass, "Should be in pass");
                    DEBUG_CHECK_EX(m_currentPass->frameBuffer.depth, "Depth stencil not used in pass");

                    auto op = allocCommand<OpClearPassDepthStencil>();
                    op->depthValue = depth;
                    op->stencilValue = stencil;
                    op->clearFlags = (doClearDepth ? 1 : 0) | (doClearStencil ? 2 : 0);
                }
            }
        }

        void CommandWriter::opClearBuffer(BufferView view, const void* clearValue /*= nullptr*/, uint32_t clearValueSize /*= 0*/)
        {
            DEBUG_CHECK_EX(view.id(), "Invalid buffer");
        }

        void CommandWriter::opClearImage(ImageView view, const void* clearValue /*= nullptr*/, uint32_t clearValueSize /*= 0*/)
        {
            DEBUG_CHECK_EX(view.id(), "Invalid image");
        }

        void CommandWriter::opResolve(const ImageView& msaaSource, const ImageView& nonMsaaDest, uint32_t sampleMask /*= INDEX_NONE*/, uint8_t depthSampleIndex /*= 0*/)
        {
            DEBUG_CHECK_EX(msaaSource, "Invalid source");
            DEBUG_CHECK_EX(nonMsaaDest, "Invalid destination");
            DEBUG_CHECK_EX(msaaSource.viewType() == ImageViewType::View2D, "Only 2D images are supported for resolve");
            DEBUG_CHECK_EX(nonMsaaDest.viewType() == ImageViewType::View2D, "Only 2D images are supported for resolve"); // HMMM, may not be necessary
            //DEBUG_CHECK_EX(msaaSource.multisampled() && msaaSource.numSamples() > 1, "Source should be multisampled");
            DEBUG_CHECK_EX(!nonMsaaDest.multisampled(), "Destination should not be multisampled");
            DEBUG_CHECK_EX(msaaSource.format() == nonMsaaDest.format(), "Source and Destination must have the same format");
            DEBUG_CHECK_EX(msaaSource.width() == nonMsaaDest.width() && msaaSource.height() == nonMsaaDest.height(), "Source and Destination must have the same size");

            if (msaaSource && msaaSource.viewType() == ImageViewType::View2D)// && msaaSource.multisampled())
            {
                if (nonMsaaDest && nonMsaaDest.viewType() == ImageViewType::View2D && !nonMsaaDest.multisampled())
                {
                    if (msaaSource.format() == nonMsaaDest.format() && msaaSource.width() == nonMsaaDest.width() && msaaSource.height() == nonMsaaDest.height())
                    {
                        auto op = allocCommand<OpResolve>();
                        op->msaaSource = msaaSource;
                        op->nonMsaaDest = nonMsaaDest;
                        op->sampleMask = sampleMask;
                        op->depthSampleIndex = depthSampleIndex;
                    }
                }
            }
        }

        void CommandWriter::opBindVertexBuffer(base::StringID bindpoint, BufferView buffer, uint32_t offset /*= 0*/)
        {
            DEBUG_CHECK_EX(bindpoint, "Invalid bind point");
            if (bindpoint)
            {
                DEBUG_CHECK_EX(buffer.id(), "Invalid vertex buffer");
                DEBUG_CHECK_EX(buffer.vertex(), "Buffer was not created with inteded use as vertex buffer");
                DEBUG_CHECK_EX(offset < buffer.size(), "Offset to vertex data is not within the buffer");

                auto op = allocCommand<OpBindVertexBuffer>();
                op->bindpoint = bindpoint;

                if (buffer.id() && buffer.vertex() && (offset < buffer.size()))
                {
                    op->offset = offset;
                    op->buffer = buffer;

                    m_currentVertexBufferRemainingSize[bindpoint] = buffer.size() - offset;
                }
                else
                {
                    op->offset = 0;
                    op->buffer = BufferView();

                    m_currentVertexBufferRemainingSize[bindpoint] = 0;
                }
            }
        }

        void CommandWriter::opUnbindVertexBuffer(base::StringID bindpoint)
        {
            DEBUG_CHECK_EX(bindpoint, "Invalid bind point");

            if (bindpoint)
            {
                auto op = allocCommand<OpBindVertexBuffer>();
                op->bindpoint = bindpoint;
                op->offset = 0;
                op->buffer = BufferView();

                m_currentVertexBufferRemainingSize[bindpoint] = 0;
            }
        }

        void CommandWriter::opBindIndexBuffer(BufferView buffer, ImageFormat indexFormat, uint32_t offset /*= 0*/)
        {
            DEBUG_CHECK_EX(buffer.id(), "Invalid index buffer");
            DEBUG_CHECK_EX(buffer.index(), "Buffer was not created with inteded use as index buffer");
            DEBUG_CHECK_EX(indexFormat == ImageFormat::R32_UINT || indexFormat == ImageFormat::R16_UINT, "Invalid index buffer format");
            DEBUG_CHECK_EX(indexFormat != ImageFormat::R32_UINT || (0 == (offset % 4)), "Index data must be aligned");
            DEBUG_CHECK_EX(indexFormat != ImageFormat::R16_UINT || (0 == (offset % 2)), "Index data must be aligned");
            DEBUG_CHECK_EX(offset < buffer.size(), "Offset to index data is not within the buffer");

            auto op = allocCommand<OpBindIndexBuffer>();

            if (buffer.id() && buffer.index() && (offset < buffer.size()) && (indexFormat == ImageFormat::R32_UINT || indexFormat == ImageFormat::R16_UINT))
            {
                op->offset = offset;
                op->buffer = buffer;
                op->format = indexFormat;

                if (indexFormat == ImageFormat::R32_UINT)
                    m_currentIndexBufferElementCount = (buffer.size() - offset) / 4;
                else
                    m_currentIndexBufferElementCount = (buffer.size() - offset) / 2;
            }
            else
            {
                op->offset = 0;
                op->format = ImageFormat::R32_UINT;
                op->buffer = BufferView();

                m_currentIndexBufferElementCount = 0;
            }
        }

        void CommandWriter::opUnbindIndexBuffer()
        {
            auto op  = allocCommand<OpBindIndexBuffer>();
            op->offset = 0;
            op->format = ImageFormat::R32_UINT;
            op->buffer = BufferView();

            m_currentIndexBufferElementCount = 0;
        }

        bool CommandWriter::validateDrawVertexLayout(const ShaderLibrary* func, uint32_t requiredVertexCount) const
        {
#ifndef BUILD_RELEASE
            ObjectID funcId;
            PipelineIndex funcIndex;
            ShaderLibraryDataPtr programData;
            if (func && func->resolve(funcId, funcIndex, &programData))
            {
                const auto& shaderBundleInfo = programData->shaderBundles()[funcIndex];

                DEBUG_CHECK_EX(shaderBundleInfo.vertexBindingState != INVALID_PIPELINE_INDEX, "Draw functions require programs with shader bundle that contains vertex input layout, even if it's empty");
                if (shaderBundleInfo.vertexBindingState == INVALID_PIPELINE_INDEX)
                    return false;

                const auto& vertexInputState = programData->vertexInputStates()[shaderBundleInfo.vertexBindingState];

                // check that we have a vertex buffer for each referenced binding point
                for (uint32_t i = 0; i < vertexInputState.numStreamLayouts; ++i)
                {
                    const auto& vertexInputLayout = programData->vertexInputLayouts()[programData->indirectIndices()[i + vertexInputState.firstStreamLayout]];
                    const auto name = programData->names()[vertexInputLayout.name];

                    uint32_t dataSizeAtVertexBinding = 0;
                    bool found = m_currentVertexBufferRemainingSize.find(name, dataSizeAtVertexBinding);
                    DEBUG_CHECK_EX(found, base::TempString("Missing vertex buffer '{}' for current draw call", name));
                    if (!found)
                        return false;

                    auto vertexSize = vertexInputLayout.customStride;
                    if (vertexSize == 0)
                    {
                        const auto& structureInfo = programData->dataLayoutStructures()[vertexInputLayout.structureIndex];
                        vertexSize = structureInfo.size;
                    }

                    auto sizeNeeded = vertexSize * requiredVertexCount;
                    DEBUG_CHECK_EX(dataSizeAtVertexBinding >= sizeNeeded, base::TempString("Vertex buffer '{}' has not enough data for current draw call {} < {}", name, dataSizeAtVertexBinding, sizeNeeded));
                    if (dataSizeAtVertexBinding < sizeNeeded)
                        return false;
                }
            }
#endif

            return true;
        }

        static ObjectViewType ViewTypeFromResourceType(ResourceType resourceType)
        {
            switch (resourceType)
            {
                case ResourceType::Buffer: return ObjectViewType::Buffer;
                case ResourceType::Texture: return ObjectViewType::Image;
                case ResourceType::Constants: return ObjectViewType::Constants;
            }

            return ObjectViewType::Invalid;
        }

        bool CommandWriter::validateParameterBindings(const ShaderLibrary* func) const
        {
#ifndef BUILD_RELEASE
            ObjectID funcId;
            PipelineIndex funcIndex;
            ShaderLibraryDataPtr programData;
            if (func && func->resolve(funcId, funcIndex, &programData))
            {
                const auto& shaderBundleInfo = programData->shaderBundles()[funcIndex];

                DEBUG_CHECK_EX(shaderBundleInfo.parameterBindingState != INVALID_PIPELINE_INDEX, "Draw/Dispatch functions require programs with parameter layout, even if it's empty");
                if (shaderBundleInfo.parameterBindingState == INVALID_PIPELINE_INDEX)
                    return false;

                const auto& parameterBindingState = programData->parameterBindingStates()[shaderBundleInfo.parameterBindingState];
                for (uint32_t i = 0; i < parameterBindingState.numParameterLayoutIndices; ++i)
                {
                    const auto& parameterLayout = programData->parameterLayouts()[parameterBindingState.firstParameterLayoutIndex + i];

                    const auto parameterBindingName = programData->names()[parameterLayout.name];

                    const auto* currentLayout = m_currentParameterBindings.find(parameterBindingName);
                    DEBUG_CHECK_EX(currentLayout != nullptr, base::TempString("Selected shader requires parameter layout '{}' that is not bound", parameterBindingName));
                    if (currentLayout == nullptr)
                        return false;

                    DEBUG_CHECK_EX(currentLayout->layout().size() == parameterLayout.numElements, base::TempString("Bound parameter layout '{}' requires {} elements but {} provided", parameterBindingName, parameterLayout.numElements, currentLayout->layout().size()));
                    if (currentLayout->layout().size() != parameterLayout.numElements)
                        return false;

                    for (uint32_t j = 0; j < parameterLayout.numElements; j++)
                    {
                        const auto& boundElemViewType = currentLayout->layout()[j];

                        const auto& expectedElem = programData->parameterLayoutsElements()[j + parameterLayout.firstElementIndex];
                        const auto expectedElemName = programData->names()[expectedElem.name];
                        const auto expectedViewType = ViewTypeFromResourceType(expectedElem.type);

                        DEBUG_CHECK_EX(expectedViewType == boundElemViewType, base::TempString("Element '{}' (index {}) at binding '{}' is expected to be {} bound actually is {}",
                            expectedElemName, j, parameterBindingName, expectedViewType, boundElemViewType));
                        if (expectedViewType != boundElemViewType)
                            return false;

                        // TODO: actual data check
                        // TODO: check for matching image flags for UAV views
                        // TODO: check for matching image format for UAV views
                        // TODO: check for matching image dimensions (2D/2DArray, etc) 
                        // TODO: check for matching buffer flags for UAV views
                        // TODO: check for matching buffer stride for structured views
                        // TODO: check for matching constant data size for constant data view
                    }
                }
            }
#endif

            return true;
        }

        void CommandWriter::opDraw(const ShaderLibrary* shader, uint32_t firstVertex, uint32_t vertexCount)
        {
            opDrawInstanced(shader, firstVertex, vertexCount, 0, 1);
        }

        void CommandWriter::opDrawInstanced(const ShaderLibrary* shader, uint32_t firstVertex, uint32_t vertexCount, uint16_t firstInstance, uint16_t numInstances)
        {
            DEBUG_CHECK_PASS_ONLY();

            DEBUG_CHECK_RETURN(shader);
            DEBUG_CHECK_RETURN(vertexCount > 0);
            DEBUG_CHECK_RETURN(numInstances > 0);

            PipelineIndex shaderIndex;
            rendering::ObjectID shaderObject;
            if (shader->resolve(shaderObject, shaderIndex))
            {
                if (validateDrawVertexLayout(shader, vertexCount))
                {
                    auto op = allocCommand<OpDraw>();
                    op->shaderLibrary = shaderObject;
                    op->shaderIndex = shaderIndex;
                    op->firstVertex = firstVertex;
                    op->vertexCount = vertexCount;
                    op->firstInstance = firstInstance;
                    op->numInstances = numInstances;
                }
            }
        }

        void CommandWriter::opDrawIndexed(const ShaderLibrary* shader, uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount)
        {
            opDrawIndexedInstanced(shader, firstVertex, firstIndex, indexCount, 0, 1);
        }

        void CommandWriter::opDrawIndexedInstanced(const ShaderLibrary* shader, uint32_t firstVertex, uint32_t firstIndex, uint32_t indexCount, uint16_t firstInstance, uint16_t numInstances)
        {
            DEBUG_CHECK_PASS_ONLY();

            DEBUG_CHECK_RETURN(shader);
            DEBUG_CHECK_RETURN(indexCount > 0);
            DEBUG_CHECK_RETURN(numInstances > 0);

            PipelineIndex shaderIndex;
            rendering::ObjectID shaderObject;

            if (shader->resolve(shaderObject, shaderIndex))
            {
                if (validateDrawVertexLayout(shader, 0))
                {
                    auto op = allocCommand<OpDrawIndexed>();
                    op->shaderLibrary = shaderObject;
                    op->shaderIndex = shaderIndex;
                    op->firstVertex = firstVertex;
                    op->firstIndex = firstIndex;
                    op->indexCount = indexCount;
                    op->firstInstance = firstInstance;
                    op->numInstances = numInstances;
                }
            }
        }

        void CommandWriter::opDispatch(const ShaderLibrary* shader, uint32_t countX /*= 1*/, uint32_t countY /*= 1*/, uint32_t countZ /*= 1*/)
        {
            DEBUG_CHECK_RETURN(shader);
            DEBUG_CHECK_RETURN(countX > 0 && countY > 0 && countZ > 0);

            PipelineIndex shaderIndex;
            rendering::ObjectID shaderObject;

            if (shader->resolve(shaderObject, shaderIndex))
            {
                auto op = allocCommand<OpDispatch>();
                op->shaderLibrary = shaderObject;
                op->shaderIndex = shaderIndex;
                op->counts[0] = countX;
                op->counts[1] = countY;
                op->counts[2] = countZ;
            }
        }

        //--

        void CommandWriter::opImageLayoutBarrier(const ImageView& view, ImageLayout layout)
        {
            DEBUG_CHECK_EX(view.id(), "Invalid image cannot be transitioned");

            if (view.id())
            {
                auto op = allocCommand<OpImageLayoutBarrier>();
                op->view = view;
                op->targetLayout = layout;
            }
        }

        void CommandWriter::opGraphicsBarrier(Stage from, Stage to)
        {
            auto op  = allocCommand<OpGraphicsBarrier>();
            op->from = from;
            op->to = to;

            if (m_currentPass)
                m_currentPass->hasBarriers = true;
        }

        //--

        void CommandWriter::opUpdateDynamicImage(const ImageView& dynamicImage, const base::image::ImageView& unsafeUpdateData, uint32_t offsetX /*= 0*/, uint32_t offsetY /*= 0*/, uint32_t offsetZ /*= 0*/)
        {
            DEBUG_CHECK_EX(dynamicImage.id(), "Unable to update invalid image");
            DEBUG_CHECK_EX(dynamicImage.dynamic(), "Dynamic update of image requires an image created with dynamic flag");
            DEBUG_CHECK_EX(!unsafeUpdateData.empty(), "Empty data for update");

            if (dynamicImage.id() && dynamicImage.dynamic() && !unsafeUpdateData.empty())
            {
                DEBUG_CHECK_EX(offsetX < dynamicImage.width() && offsetY < dynamicImage.height() && offsetZ < dynamicImage.depth(), "Update offset is outside image space, something is seriously wrong");
                if (offsetX < dynamicImage.width() && offsetY < dynamicImage.height() && offsetZ < dynamicImage.depth())
                {
                    auto maxAllowedWidth = dynamicImage.width() - offsetX;
                    auto maxAllowedHeight = dynamicImage.height() - offsetY;
                    auto maxAllowedDepth = dynamicImage.depth() - offsetZ;

                    DEBUG_CHECK_EX(unsafeUpdateData.width() <= maxAllowedWidth && unsafeUpdateData.height() <= maxAllowedHeight && unsafeUpdateData.depth() <= maxAllowedDepth, "Update data placement goes outside the target image");

                    auto safeUpdateData = unsafeUpdateData.subView(0, 0, 0,
                        std::min<uint32_t>(maxAllowedWidth, unsafeUpdateData.width()),
                        std::min<uint32_t>(maxAllowedHeight, unsafeUpdateData.height()),
                        std::min<uint32_t>(maxAllowedDepth, unsafeUpdateData.depth()));

                    auto dataSize = safeUpdateData.dataSize();
                    auto maxLocalPayload = std::max<uint32_t>(1024, m_writeEndPtr - m_writePtr);

                    OpUpdateDynamicImage* op;
                    if (dataSize < maxLocalPayload)
                    {
                        op = allocCommand<OpUpdateDynamicImage>(dataSize);
                        op->dataBlockPtr = op->payload();
                    }
                    else
                    {
                        op = allocCommand<OpUpdateDynamicImage>();
                        op->dataBlockPtr = m_writeBuffer->m_pages->allocateOustandingBlock(dataSize, 16);
                    }

                    op->view = dynamicImage;
                    op->data = base::image::ImageView(base::image::NATIVE_LAYOUT, safeUpdateData.format(), safeUpdateData.channels(), op->dataBlockPtr, safeUpdateData.width(), safeUpdateData.height(), safeUpdateData.depth());
                    op->placementOffset[0] = offsetX;
                    op->placementOffset[1] = offsetY;
                    op->placementOffset[2] = offsetZ;
                    base::image::Copy(safeUpdateData, op->data);
                }
            }
        }

        void* CommandWriter::opUpdateDynamicBufferPtr(const BufferView& dynamicBuffer, uint32_t dataOffset, uint32_t dataSize)
        {
            DEBUG_CHECK_EX(dynamicBuffer.id(), "Unable to update invalid buffer");
            DEBUG_CHECK_EX(dynamicBuffer.dynamic(), "Dynamic update of buffer requires a buffer created with dynamic flag");
            
            if (dynamicBuffer.id() && dynamicBuffer.dynamic() && dataSize)
            {
                DEBUG_CHECK_EX(dataOffset < dynamicBuffer.size(), "Update offset is not within buffer bounds, something is really wrong");
                if (dataOffset < dynamicBuffer.size())
                {
                    auto maxUpdateSize = dynamicBuffer.size() - dataOffset;
                    DEBUG_CHECK_EX(dataSize <= maxUpdateSize, "Dynamic data to update goes over the buffer range");

                    dataSize = std::min<uint32_t>(maxUpdateSize, dataSize);
                    auto maxLocalPayload = std::max<uint32_t>(1024, m_writeEndPtr - m_writePtr);

                    OpUpdateDynamicBuffer* op;
                    if (dataSize < maxLocalPayload)
                    {
                        op = allocCommand<OpUpdateDynamicBuffer>(dataSize);
                        op->dataBlockPtr = op->payload();
                    }
                    else
                    {
                        op = allocCommand<OpUpdateDynamicBuffer>();
                        op->dataBlockPtr = m_writeBuffer->m_pages->allocateOustandingBlock(dataSize, 16);
                    }

                    //memcpy(, dataPtr, dataSize);
                    op->view = dynamicBuffer;
                    op->offset = dataOffset;
                    op->dataBlockSize = dataSize;
                    op->stagingBufferOffset = 0;
                    op->next = nullptr;

                    if (m_writeBuffer->m_gatheredState.dynamicBufferUpdatesHead == nullptr)
                        m_writeBuffer->m_gatheredState.dynamicBufferUpdatesHead = op;
                    else
                        m_writeBuffer->m_gatheredState.dynamicBufferUpdatesTail->next = op;
                    m_writeBuffer->m_gatheredState.dynamicBufferUpdatesTail = op;

                    return op->dataBlockPtr;
                }
            }

            return nullptr;
        }

        void CommandWriter::opUpdateDynamicBuffer(const BufferView& dynamicBuffer, uint32_t dataOffset, uint32_t dataSize, const void* dataPtr)
        {
            if (dataSize)
            {
                DEBUG_CHECK_EX(dataPtr, "Empty data for update");

                if (auto* targetPtr = opUpdateDynamicBufferPtr(dynamicBuffer, dataOffset, dataSize))
                    memcpy(targetPtr, dataPtr, dataSize);
            }
        }

        //---

        void CommandWriter::opCopyBuffer(const BufferView& src, const BufferView& dest, uint32_t srcOffset, uint32_t destOffset, uint32_t size)
        {
            DEBUG_CHECK_EX(size > 0, "No data to copy");
            DEBUG_CHECK_EX(src.id(), "Invalid source buffer");
            DEBUG_CHECK_EX(dest.id(), "Invalid target buffer");
            DEBUG_CHECK_EX(src.copyCapable(), "Only copiable buffers can be copied. DUH.");
            DEBUG_CHECK_EX(dest.copyCapable(), "Only copiable buffers can be copied. DUH.");
            DEBUG_CHECK_EX(src.offset() + srcOffset + size < src.size(), "Source region out of bounds");
            DEBUG_CHECK_EX(dest.offset() + destOffset + size < dest.size(), "Destination region out of bounds");

            if (src.id() == dest.id())
            {
                const auto srcAbsStart = src.offset() + srcOffset;
                const auto srcAbsEnd = srcAbsStart + size;
                const auto destAbsStart = dest.offset() + destOffset;
                const auto destAbsEnd = destAbsStart + size;
                DEBUG_CHECK_EX(destAbsEnd <= srcAbsStart || destAbsStart >= srcAbsEnd, "Overlaped buffer regions");
            }
        }

        //---

        void CommandWriter::opDownloadBuffer(const BufferView& buffer, const DownloadBufferPtr& ptr)
        {
            DEBUG_CHECK_EX(buffer.id(), "Cannot copy from invalid buffer");
            DEBUG_CHECK_EX(buffer.copyCapable(), "Buffer to copy from must be copy capable :)");

            if (buffer.id() && buffer.copyCapable() && ptr)
            {
                auto op = allocCommand<OpDownloadBuffer>();
                memset(&op->ptr, 0, sizeof(op->ptr));
                op->buffer = buffer;
                op->downloadOffset = 0;
                op->downloadSize = buffer.size();
                op->ptr = ptr;

                m_writeBuffer->m_downloadBuffers.pushBack(ptr);
            }
        }

        void CommandWriter::opDownloadImage(const ImageView& image, const DownloadImagePtr& ptr)
        {
            DEBUG_CHECK_EX(image.id(), "Cannot copy from invalid image");
            DEBUG_CHECK_EX(image.copyCapable(), "Image to copy from must be copy capable :)");

            if (image.id() && image.copyCapable() && ptr)
            {
                auto op = allocCommand<OpDownloadImage>();
                memset(&op->ptr, 0, sizeof(op->ptr));
                op->image = image;
                op->ptr = ptr;

                m_writeBuffer->m_downloadImages.pushBack(ptr);
            }
        }

        //---

        ConstantsView CommandWriter::opAllocConstants(uint32_t size, void*& outDataPtr)
        {
            ASSERT_EX(size > 0, "Recording contants with zero size is not a good idea");

            // allocate space
            auto uploadSpaceOffset = base::Align<uint32_t>(m_writeBuffer->m_gatheredState.totalConstantsUploadSize, 256);
            m_writeBuffer->m_gatheredState.totalConstantsUploadSize = uploadSpaceOffset + base::Align<uint32_t>(size, 16);

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

            op->offset = uploadSpaceOffset;
            op->dataSize = alignedSize;
            op->nextConstants = nullptr;

            // get the pointer to the data offset
            auto offsetPtr  = &op->mergedRuntimeOffset;
            op->mergedRuntimeOffset = 0xBAADF00D;

            // link in the list
            if (m_writeBuffer->m_gatheredState.constantUploadTail)
                m_writeBuffer->m_gatheredState.constantUploadTail->nextConstants = op;
            else
                m_writeBuffer->m_gatheredState.constantUploadHead = op;
            m_writeBuffer->m_gatheredState.constantUploadTail = op;
            outDataPtr = op->dataPtr;

            // return constant view at given offset
            return ConstantsView(offsetPtr, 0, alignedSize);
        }

        ConstantsView CommandWriter::opUploadConstants(const void* data, uint32_t size)
        {
            ASSERT_EX(size > 0, "Recording contants with zero size is not a good idea");

            // allocate data
            void* targetDataPtr = nullptr;
            auto view = opAllocConstants(size, targetDataPtr);

            // copy data
            if (!view.empty() && size)
                memcpy(targetDataPtr, data, size);

            // return constant view at given offset
            return view;
        }

        ParametersView CommandWriter::opUploadParameters(const void* data, uint32_t dataSize, ParametersLayoutID layoutID)
        {
            ASSERT_EX(layoutID.value() < CommandBufferGatheredState::MAX_LAYOUTS, "To many different parameter layouts");

            // invalid layout
            if (layoutID.empty())
                return ParametersView();

            // get the layout information
            auto& layoutInfo = layoutID.layout();
            if (!layoutInfo.validateMemory(data, dataSize))
                return ParametersView();

            // validate resource bindings
            if (!layoutInfo.validateBindings(data))
                return ParametersView();

            // allocate the OP and copy the data in there
            auto op  = allocCommand<OpUploadParameters>(layoutInfo.memorySize());
            op->layout = &layoutInfo;
            op->nextParameters = nullptr;
            memcpy(op->payload(), data, layoutInfo.memorySize());

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

            // return the view on the data
            return ParametersView(op->payload(), layoutID, op->index);
        }

        //---

        void CommandWriter::opBindParameters(base::StringID binding, const ParametersView& parameters)
        {
            DEBUG_CHECK_EX(binding, "Unnamed binding for parameters");

            if (parameters)
            {
                // validate the data we are binding
#ifndef BUILD_RELEASE
                parameters.layout().layout().validateMemory(parameters.dataPtr(), parameters.layout().memorySize());
#endif

                // TODO: filtering ?
                auto op = allocCommand<OpBindParameters>();
                op->binding = binding;
                op->view = parameters;

#ifndef BUILD_RELEASE
                m_currentParameterBindings[binding] = parameters.layout();
#endif
            }
            else
            {
                auto op = allocCommand<OpBindParameters>();
                op->binding = binding;
                op->view = ParametersView();

#ifndef BUILD_RELEASE
                if (auto* value = m_currentParameterBindings.find(binding))
                    *value = ParametersLayoutID(); // empty layout
#endif
            }
        }

        //---

    } // command
}  // rendering
