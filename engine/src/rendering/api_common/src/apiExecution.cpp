/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiObject.h"
#include "apiThread.h"
#include "apiFrame.h"
#include "apiExecution.h"
#include "apiObjectCache.h"
#include "apiObjectRegistry.h"
#include "apiTransientBuffer.h"
#include "apiOutput.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingCommands.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingDescriptorInfo.h"
#include "rendering/device/include/renderingDescriptor.h"


namespace rendering
{
    namespace api
    {
		//--

		RuntimeDataAllocations::RuntimeDataAllocations()
		{}

		uint32_t RuntimeDataAllocations::allocStagingData(uint32_t size)
		{
			auto offset = base::Align<uint32_t>(m_requiredStagingBuffer, 256);
			m_requiredStagingBuffer = offset + size;
			return offset;
		}

		void RuntimeDataAllocations::reportConstantsBlockSize(uint32_t size)
		{
			auto constsOffset = base::Align<uint32_t>(m_requiredConstantsBuffer, 256);
			m_requiredConstantsBuffer = constsOffset + size;
			m_constantsDataOffsetInStaging = allocStagingData(size);

			auto& copy = m_constantBufferCopies.emplaceBack();
			copy.size = size;
			copy.sourceOffset = m_constantsDataOffsetInStaging;
			copy.targetOffset = constsOffset;
		}

		void RuntimeDataAllocations::reportConstData(uint32_t offset, uint32_t size, const void* dataPtr, uint32_t& outOffsetInBigBuffer)
		{
			ASSERT(size > 0);
			ASSERT(dataPtr != nullptr);

			auto& write = m_writes.emplaceBack();
			write.size = size;
			write.offset = offset + m_constantsDataOffsetInStaging;
			write.data = dataPtr;

			outOffsetInBigBuffer = m_constantsDataOffsetInStaging + offset;
		}

		void RuntimeDataAllocations::reportBufferUpdate(const void* updateData, uint32_t updateSize, uint32_t& outStagingOffset)
		{
			ASSERT(updateSize > 0);
			ASSERT(updateData != nullptr);

			// allocate space in the storage
			outStagingOffset = base::Align<uint32_t>(m_requiredStagingBuffer, 16);
			m_requiredStagingBuffer = outStagingOffset + updateSize;

			// write update data to storage
			auto& write = m_writes.emplaceBack();
			write.offset = outStagingOffset;
			write.size = updateSize;
			write.data = updateData;

			ASSERT((uint64_t)updateData >= 0x1000000);
		}

        //---

		DynamicRenderStates::DynamicRenderStates()
		{}

		static DynamicRenderStates GDefaultStats;

		const DynamicRenderStates& DynamicRenderStates::DEFAULT_STATES()
		{
			return GDefaultStats;
		}

		//---

		GeometryBufferBinding::GeometryBufferBinding()
		{}

		void GeometryBufferBinding::print(base::IFormatStream& f) const
		{
			if (id)
				f.appendf("{} at offset {}", id, offset);
			else
				f.append("EMPTY");
		}
		
		GeometryStates::GeometryStates()
		{
		}

		void GeometryStates::print(base::IFormatStream& f) const
		{
			f.appendf("INDEX: {}, format {}, total offset: {}\n", indexBinding, indexFormat, finalIndexStreamOffset);

			for (auto i : vertexBindings.indexRange())
				f.appendf("VERTEX[{}]: {}\n", i, vertexBindings[i]);
		}

		//---

		DescriptorBinding::DescriptorBinding()
		{}

		void DescriptorBinding::print(base::IFormatStream& f) const
		{
			if (dataPtr)
				f.appendf("{}: '{}' at {}", name, layoutPtr->id(), dataPtr);
			else if (name)
				f.appendf("{}: EMPTY", name);
			else
				f << "UNKNOWN";
		}

		//---

		void DescriptorState::print(base::IFormatStream& f) const
		{
			for (auto i : descriptors.indexRange())
				f.appendf("[{}]: {}\n", i, descriptors[i]);
		}

		//---

		PassAttachmentBinding::PassAttachmentBinding()
		{}

		void PassAttachmentBinding::print(base::IFormatStream& f) const
		{
			if (viewId)
				f.appendf("{} [{}x{}] {}, {} samples, {}, {}", viewId, width, height, format, samples, loadOp, storeOp);
			else
				f << "EMPTY";
		}

		//---

		PassState::PassState()
		{}

		void PassState::print(base::IFormatStream& f) const
		{
			f.appendf("PASS: {} color RTs, {} viewports\n", colorCount, viewportCount);
			f.appendf("  DEPTH: {}\n", depth);
			for (uint32_t i = 0; i < colorCount; ++i)
				f.appendf("  COLOR[{}]: {}\n", i, color[i]);
		}

		//--

		IBaseFrameExecutor::IBaseFrameExecutor(IBaseThread* thread, Frame* frame, PerformanceStats* stats)
			: m_thread(thread)
			, m_frame(frame)
			, m_objectRegistry(thread->objectRegistry())
			, m_objectCache(thread->objectCache())
			, m_stats(stats)
		{
		}

		IBaseFrameExecutor::~IBaseFrameExecutor()
		{
		}
		
		void IBaseFrameExecutor::execute(command::CommandBuffer* commandBuffer)
		{
			PC_SCOPE_LVL1(ExecuteCommandBuffer);

			// TODO: different execution models (multiple native command buffers, etc)
			{
				base::ScopeTimer commandTimer;
				executeSingle(commandBuffer);
				m_stats->executionTime = commandTimer.timeElapsed();
			}
		}

		void IBaseFrameExecutor::executeSingle(command::CommandBuffer* commandBuffer)
		{
			uint32_t numCommands = 0;

			auto* cmd = commandBuffer->commands();
			while (cmd)
			{
				switch (cmd->op)
				{
#define RENDER_COMMAND_OPCODE(x) case command::CommandCode::##x: { run##x(*static_cast<const command::Op##x*>(cmd)); break; }
#include "rendering/device/include/renderingCommandOpcodes.inl"
#undef RENDER_COMMAND_OPCODE
				default:
					DEBUG_CHECK(!"Unsupported command recorded");
					break;
				}

				cmd = command::GetNextCommand(cmd);
				numCommands += 1;
			}

			m_stats->numLogicalCommandBuffers += 1;
			m_stats->numCommands += numCommands;
		}

		//--

		IFrameExecutor::IFrameExecutor(IBaseThread* thread, Frame* frame, PerformanceStats* stats)
			: IBaseFrameExecutor(thread, frame, stats)
		{
		}

		IFrameExecutor::~IFrameExecutor()
		{
			DEBUG_CHECK_EX(!m_activeSwapchain, "Some acquired outputs were not swapped!");
		}

		void IFrameExecutor::pushDescriptorState(bool inheritCurrentParameters)
		{
			if (inheritCurrentParameters)
			{
				m_descriptorStateStack.emplaceBack(m_descriptors);
			}
			else
			{
				m_descriptorStateStack.emplaceBack(std::move(m_descriptors));
				m_descriptors.descriptors.reset();
				m_dirtyDescriptors = true;
			}
		}

		void IFrameExecutor::popDescriptorState()
		{
			ASSERT_EX(!m_descriptorStateStack.empty(), "Param stack underflow");

			m_descriptors = std::move(m_descriptorStateStack.back());
			m_descriptorStateStack.popBack();

			m_dirtyDescriptors = true;
		}

		//--

		void IFrameExecutor::prepare(const RuntimeDataAllocations& info)
		{
			PC_SCOPE_LVL2(PrepareFrameData);

			// create buffers
			{
				PC_SCOPE_LVL2(Allocate);

				if (info.m_requiredConstantsBuffer)
				{
					auto buffer = thread()->transientConstantPool()->allocate(info.m_requiredConstantsBuffer);
					frame()->registerCompletionCallback([buffer]() { buffer->returnToPool(); });
					m_constantBuffer = buffer;

				}

				if (info.m_requiredStagingBuffer)
				{
					auto buffer = thread()->transientStagingPool()->allocate(info.m_requiredStagingBuffer);
					frame()->registerCompletionCallback([buffer]() { buffer->returnToPool(); });
					m_stagingBuffer = buffer;
				}
			}

			// upload data to staging buffer from command buffer
			{
				PC_SCOPE_LVL2(Write);

				for (auto& write : info.m_writes)
					m_stagingBuffer->writeData(write.offset, write.size, write.data);
			}

			// finish writes
			if (m_stagingBuffer)
			{
				PC_SCOPE_LVL2(Flush);
				m_stagingBuffer->flush();
			}

			// copy data to other buffers
			{
				PC_SCOPE_LVL2(CopyBuffers);
				for (auto& copy : info.m_constantBufferCopies)
					m_constantBuffer->copyDataFrom(m_stagingBuffer, copy.sourceOffset, copy.targetOffset, copy.size);

				// end copying
				//m_tempConstantBuffer->flushCopies();
			}
		}

		//--

		void IFrameExecutor::runNop(const command::OpNop& op)
		{
		}

		void IFrameExecutor::runHello(const command::OpHello& op)
		{
		}

		void IFrameExecutor::runNewBuffer(const command::OpNewBuffer& op)
		{
		}

		void IFrameExecutor::runTriggerCapture(const command::OpTriggerCapture& op)
		{
		}

		void IFrameExecutor::runAcquireOutput(const command::OpAcquireOutput& op)
		{
			ASSERT(!m_activeSwapchain);
			ASSERT(op.output);
			m_activeSwapchain = op.output;
			m_activeSwapchainAcquired = false;
		}

		void IFrameExecutor::runSwapOutput(const command::OpSwapOutput& op)
		{
			ASSERT(m_activeSwapchain);
			ASSERT(m_activeSwapchain == op.output);

			auto* output = objects()->resolveStatic<Output>(m_activeSwapchain);
			DEBUG_CHECK_EX(output, "Output window deleted");

			if (output && m_activeSwapchainAcquired)
				output->present(op.swap);

			m_activeSwapchain = ObjectID();
			m_activeSwapchainAcquired = false;
		}

		void IFrameExecutor::runChildBuffer(const command::OpChildBuffer& op)
		{
			pushDescriptorState(op.inheritsParameters);
			executeSingle(op.childBuffer);
			popDescriptorState();
		}

		void IFrameExecutor::runUploadConstants(const command::OpUploadConstants& op)
		{
			// nothing here, this op is placeholder
		}

		void IFrameExecutor::runUploadDescriptor(const command::OpUploadDescriptor& op)
		{
			// nothing here, this op is placeholder
		}

		//--

		void IFrameExecutor::runBindVertexBuffer(const command::OpBindVertexBuffer& op)
		{
			if (op.bindpoint)
			{
				auto bindPointIndex = cache()->resolveVertexBindPointIndex(op.bindpoint);
				m_geometry.vertexBindings.prepare(bindPointIndex + 1);

				auto& binding = m_geometry.vertexBindings[bindPointIndex];
				if (binding.id != op.id || binding.offset != op.offset)
				{
					binding.id = op.id;
					binding.offset = op.offset;

					m_dirtyVertexBuffers = true;
				}
			}
		}

		void IFrameExecutor::runBindIndexBuffer(const command::OpBindIndexBuffer& op)
		{
			auto& binding = m_geometry.indexBinding;
			if (binding.id != op.id || binding.offset != op.offset || m_geometry.indexFormat != op.format)
			{
				binding.id = op.id;
				binding.offset = op.offset;

				m_geometry.indexFormat = op.format;
				m_dirtyIndexBuffer = true;
			}
		}

		uint32_t IFrameExecutor::printBoundDescriptors() const
		{
			uint32_t count = 0;

			TRACE_INFO("Currently known {} parameter bindings points", m_descriptors.descriptors.size());
			for (auto i : m_descriptors.descriptors.indexRange())
			{
				const auto& desc = m_descriptors.descriptors[i];
				TRACE_INFO("  [{}]: {} {}", i, desc.layoutPtr->id(), desc.dataPtr);
				count += 1;
			}

			return count;
		}

		void IFrameExecutor::runBindDescriptor(const command::OpBindDescriptor& op)
		{
			ASSERT(op.binding);
			ASSERT(op.layout);

			auto bindingIndex = cache()->resolveDescriptorBindPointIndex(op.binding, op.layout->id());
			m_descriptors.descriptors.prepare(bindingIndex + 1);

			auto& descriptor = m_descriptors.descriptors[bindingIndex];

			if (descriptor.dataPtr != op.data || descriptor.layoutPtr != op.layout)
			{
				// TODO: if layout is the same maybe we should check for same data ?

				descriptor.dataPtr = op.data;
				descriptor.layoutPtr = op.layout;
				m_dirtyDescriptors = true;
			}
		}

		//--

		void IFrameExecutor::runSetViewportRect(const command::OpSetViewportRect& op)
		{
			ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");
			ASSERT_EX(op.viewportIndex < m_pass.viewportCount, "Viewport was not enabled in pass");

			auto& v = m_dynamic.viewports[op.viewportIndex];

			const float x = op.rect.left();
			//const float y = op.rect.top();
			const float y = (int)m_pass.height - op.rect.top() - op.rect.height();
			const float w = op.rect.width();
			const float h = op.rect.height();

			if (v.rect[0] != x || v.rect[1] != y || v.rect[2] != w || v.rect[3] != h || v.depthMin != op.depthMin || v.depthMax != op.depthMax)
			{
				v.rect[0] = x;
				v.rect[1] = y;
				v.rect[2] = w;
				v.rect[3] = h;
				v.depthMin = op.depthMin;
				v.depthMax = op.depthMax;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::ViewportRects;
			}
		}

		void IFrameExecutor::runSetLineWidth(const command::OpSetLineWidth& op)
		{
			ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

			if (op.width != m_dynamic.lineWidth)
			{
				m_dynamic.lineWidth = op.width;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::LineWidth;
			}
		}

		void IFrameExecutor::runSetDepthBias(const command::OpSetDepthBias& op)
		{
			ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

			if (op.clamp != m_dynamic.depthBiasClamp || op.constant != m_dynamic.depthBiasConstant || op.slope != m_dynamic.depthBiasSlope)
			{
				m_dynamic.depthBiasClamp = op.clamp;
				m_dynamic.depthBiasConstant = op.constant;
				m_dynamic.depthBiasSlope = op.slope;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::DepthBiasValues;
			}
		}

		void IFrameExecutor::runSetDepthClip(const command::OpSetDepthClip& op)
		{
			ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

			if (op.min != m_dynamic.depthClipMin || op.max != m_dynamic.depthClipMax)
			{
				m_dynamic.depthClipMin = op.min;
				m_dynamic.depthClipMax = op.max;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::DepthClampValues;
			}
		}

		void IFrameExecutor::runSetBlendColor(const command::OpSetBlendColor& op)
		{
			ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

			if (op.color[0] != m_dynamic.blendColor[0] || op.color[1] != m_dynamic.blendColor[1] 
				|| op.color[2] != m_dynamic.blendColor[2] || op.color[3] != m_dynamic.blendColor[3])
			{
				m_dynamic.blendColor[0] = op.color[0];
				m_dynamic.blendColor[1] = op.color[1];
				m_dynamic.blendColor[2] = op.color[2];
				m_dynamic.blendColor[3] = op.color[3];
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::BlendColor;
				
			}
		}
			
		void IFrameExecutor::runSetScissorRect(const command::OpSetScissorRect& op)
		{
			ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");
			ASSERT_EX(op.viewportIndex < m_pass.viewportCount, "Viewport was not enabled in pass");

			auto& targetRect = m_dynamic.viewports[op.viewportIndex].scissor;

			const auto x = op.rect.left();
			const auto y = (int)m_pass.height - op.rect.top() - op.rect.height();
			//const auto y = op.rect.top();
			const auto w = op.rect.width();
			const auto h = op.rect.height();

			if (x != targetRect[0] || y != targetRect[1] || w != targetRect[2] || h != targetRect[3])
			{
				targetRect[0] = x;
				targetRect[1] = y;
				targetRect[2] = w;
				targetRect[3] = h;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::ScissorRects;
			}
		}

		void IFrameExecutor::runSetStencilReference(const command::OpSetStencilReference& op)
		{
			ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

			if (op.front != m_dynamic.stencilFrontReference)
			{
				m_dynamic.stencilFrontReference = op.front;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::StencilFrontRef;
			}

			if (op.back != m_dynamic.stencilBackReference)
			{
				m_dynamic.stencilBackReference = op.back;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::StencilBackRef;
			}
		}

		void IFrameExecutor::runSetStencilWriteMask(const command::OpSetStencilWriteMask& op)
		{
			ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

			if (op.front != m_dynamic.stencilFrontWriteMask)
			{
				m_dynamic.stencilFrontWriteMask = op.front;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::StencilFrontWriteMask;
			}

			if (op.back != m_dynamic.stencilBackWriteMask)
			{
				m_dynamic.stencilBackWriteMask = op.back;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::StencilBackWriteMask;
			}
		}

		void IFrameExecutor::runSetStencilCompareMask(const command::OpSetStencilCompareMask& op)
		{
			ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

			if (op.front != m_dynamic.stencilFrontCompareMask)
			{
				m_dynamic.stencilFrontCompareMask = op.front;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::StencilFrontCompareMask;
			}

			if (op.back != m_dynamic.stencilBackCompareMask)
			{
				m_dynamic.stencilBackCompareMask = op.back;
				m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::StencilBackCompareMask;
			}
		}
		
		//--

		static bool IsSwapChain(const FrameBuffer& fb)
		{
			return fb.color[0].swapchain || fb.depth.swapchain;
		}

		// clears
		struct ColorClearInfo
		{
			LoadOp loadOp;
			bool m_clear;

			union
			{
				float m_float[4];
				uint32_t m_uint[4];
			};

			inline ColorClearInfo()
				: m_clear(false)
			{}
		};

		void IFrameExecutor::ExtractPassAttachment(PassState& state, int index, PassAttachmentBinding& target, const FrameBufferColorAttachmentInfo& att)
		{
			if (state.width == 0 && state.height == 0 && state.samples == 0)
			{
				state.width = att.width;
				state.height = att.height;
				state.samples = att.samples;
				state.swapchain = att.swapchain;
			}
			else
			{
				ASSERT_EX(att.width == state.width && att.height == state.height, "Render targets have different sizes");
				ASSERT_EX(att.samples == state.samples, "Render targets have different sample count");
				ASSERT_EX(att.swapchain == state.swapchain, "Cannot mix swapchain and non-swapchain surfaces");
			}

			target.viewId = att.viewID;
			target.loadOp = att.loadOp;
			target.storeOp = att.storeOp;

			if (att.loadOp == LoadOp::Clear)
			{
				target.clearValue.valueFloat[0] = att.clearColorValues[0];
				target.clearValue.valueFloat[1] = att.clearColorValues[1];
				target.clearValue.valueFloat[2] = att.clearColorValues[2];
				target.clearValue.valueFloat[3] = att.clearColorValues[3];
			}
		}

		void IFrameExecutor::ExtractPassAttachment(PassState& state, PassAttachmentBinding& target, const FrameBufferDepthAttachmentInfo& att)
		{
			if (state.width == 0 && state.height == 0 && state.samples == 0)
			{
				state.width = att.width;
				state.height = att.height;
				state.samples = att.samples;
				state.swapchain = att.swapchain;
			}
			else
			{
				ASSERT_EX(att.width == state.width && att.height == state.height, "Render targets have different sizes");
				ASSERT_EX(att.samples == state.samples, "Render targets have different sample count");
				ASSERT_EX(att.swapchain == state.swapchain, "Cannot mix swapchain and non-swapchain surfaces");
			}

			target.viewId = att.viewID;
			target.loadOp = att.loadOp;
			target.storeOp = att.storeOp;
			target.width = state.width;
			target.height= state.height;
			target.samples = state.samples;

			if (att.loadOp == LoadOp::Clear)
			{
				target.clearValue.valueFloat[0] = att.clearDepthValue;
				target.clearValue.valueUint[1] = att.clearStencilValue;
			}
		}

		void IFrameExecutor::runBeginPass(const command::OpBeginPass& op)
		{
			ASSERT_EX(op.passLayoutId, "Invalid pass layout");
			ASSERT_EX(op.frameBuffer.validate(), "Begin pass with invalid frame buffer should not be recorded");
			ASSERT_EX(op.numViewports >= 1 && op.numViewports <= 16, "Invalid viewport count");
			ASSERT_EX(!m_pass.passOp, "Theres already an active pass");
			ASSERT_EX(!m_activePassLayout, "Theres already an active pass");

			// determine rendering area size as we go
			memzero(&m_pass, sizeof(m_pass));

			// extract depth attachment
			if (op.frameBuffer.depth)
				ExtractPassAttachment(m_pass, m_pass.depth, op.frameBuffer.depth);

			// extract color attachments
			for (uint32_t i = 0; i < FrameBuffer::MAX_COLOR_TARGETS; ++i)
			{
				const auto& att = op.frameBuffer.color[i];
				if (att.empty())
					break; // last color

				auto& target = m_pass.color[m_pass.colorCount++];
				ExtractPassAttachment(m_pass, i, target, att);
			}

			// set viewports/scissor rects from passed data
			if (op.hasInitialViewportSetup)
			{
				m_pass.viewportCount = op.numViewports;

				const auto* viewPtr = (const FrameBufferViewportState*)op.payload();
				for (uint8_t i = 0; i < op.numViewports; ++i, ++viewPtr)
				{
					auto& v = m_dynamic.viewports[i];
					if (viewPtr->viewportRect.empty())
					{
						v.rect[0] = 0;
						v.rect[1] = 0;
						v.rect[2] = m_pass.width;
						v.rect[3] = m_pass.height;
					}
					else
					{
						v.rect[0] = viewPtr->viewportRect.min.x;
						v.rect[1] = viewPtr->viewportRect.min.y;
						v.rect[2] = viewPtr->viewportRect.max.x;
						v.rect[3] = viewPtr->viewportRect.max.y;
					}

					if (viewPtr->scissorRect.empty())
					{
						v.scissor[0] = v.rect[0];
						v.scissor[1] = v.rect[1];
						v.scissor[2] = v.rect[2];
						v.scissor[3] = v.rect[3];
					}
					else
					{
						v.scissor[0] = viewPtr->scissorRect.min.x;
						v.scissor[1] = viewPtr->scissorRect.min.y;
						v.scissor[2] = viewPtr->scissorRect.max.x;
						v.scissor[3] = viewPtr->scissorRect.max.y;
					}

					v.depthMin = viewPtr->minDepthRange;
					v.depthMax = viewPtr->maxDepthRange;
				}
			}
			else
			{
				m_pass.viewportCount = 1;

				auto& v = m_dynamic.viewports[0];
				v.rect[0] = 0;
				v.rect[1] = 0;
				v.rect[2] = m_pass.width;
				v.rect[3] = m_pass.height;
				v.scissor[0] = 0;
				v.scissor[1] = 0;
				v.scissor[2] = m_pass.width;
				v.scissor[3] = m_pass.height;
				v.depthMin = 0.0f;
				v.depthMax = 1.0f;
			}

			// setting a new pass resets viewports and scissor stuff
			m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::ViewportRects;
			m_drawDirtyRenderStates |= DynamicRenderStatesDirtyBit::ScissorRects;

			// use given pass layout
			m_activePassLayout = op.passLayoutId;

			// if rendering to swapchain surfaces make sure we acquire the output
			// NOTE: this is done only at first rendering to those surfaces
			if (m_pass.swapchain)
			{
				ASSERT_EX(m_activeSwapchain, "No swapchain bound");

				if (!m_activeSwapchainAcquired)
				{
					auto* output = objects()->resolveStatic<Output>(m_activeSwapchain);
					DEBUG_CHECK_EX(output, "Output window deleted");

					if (output)
					{
						m_activeSwapchainAcquired = output->acquire();
						DEBUG_CHECK_EX(m_activeSwapchainAcquired, "Failed to acquire output");
					}
				}
			}

			// clear graphics state tracking for this frame
			m_drawDirtyRenderStates.clearAll();
			m_passDirtyRenderStates.clearAll();
		}

		void IFrameExecutor::runEndPass(const command::OpEndPass& op)
		{
			DEBUG_CHECK_EX(m_pass.passOp, "No pass active");

			m_pass = PassState();

			if (m_passDirtyRenderStates.rawValue())
			{
				applyDynamicStates(m_dynamic, m_passDirtyRenderStates);

				m_dynamic = DynamicRenderStates::DEFAULT_STATES();
				m_passDirtyRenderStates.clearAll();
				m_drawDirtyRenderStates.clearAll();
			}
		}

		void IFrameExecutor::flushDrawDynamicStates()
		{
			if (m_drawDirtyRenderStates.rawValue())
			{
				applyDynamicStates(m_dynamic, m_drawDirtyRenderStates);
				m_passDirtyRenderStates |= m_drawDirtyRenderStates;
				m_drawDirtyRenderStates.clearAll();
			}
		}

		//--

    } // api
} // rendering
