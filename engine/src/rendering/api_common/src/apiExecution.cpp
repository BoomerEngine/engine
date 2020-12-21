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
#include "apiExecution.h"
#include "apiObjectCache.h"
#include "apiObjectRegistry.h"
#include "apiOutput.h"
#include "apiImage.h"
#include "apiBuffer.h"
#include "apiDownloadArea.h"

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

		FrameExecutionData::FrameExecutionData()
		{}

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

		IBaseFrameExecutor::IBaseFrameExecutor(IBaseThread* thread, PerformanceStats* stats)
			: m_thread(thread)
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

		IFrameExecutor::IFrameExecutor(IBaseThread* thread, PerformanceStats* stats)
			: IBaseFrameExecutor(thread, stats)
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

		void IFrameExecutor::runDownload(const command::OpDownload& op)
		{
			auto destObject = objects()->resolveStatic(op.id, ObjectType::Unknown);
			DEBUG_CHECK_RETURN_EX(destObject, "Destination object lost before command buffer was run (waited more than one frame for submission)");

			auto* destCopiable = destObject->toCopiable();
			DEBUG_CHECK_RETURN_EX(destCopiable, "Destination object is not copiable");

			auto* destArea = objects()->resolveStatic<IBaseDownloadArea>(op.areaId);
			DEBUG_CHECK_RETURN_EX(destArea, "Destination area is invalid");

			destCopiable->downloadIntoArea(destArea, op.offsetInArea, op.sizeInArea, op.range);

			{
				auto areaPtr = DownloadAreaObjectPtr(AddRef(op.area));
				auto sinkPtr = DownloadDataSinkPtr(AddRef(op.sink));
				auto size = op.sizeInArea;
				auto offset = op.offsetInArea;
				auto range = op.range;

				thread()->registerCurrentFrameGPUComplectionCallback([areaPtr, sinkPtr, size, offset, range]() {
					const auto* dataPtr = areaPtr->memoryPointer() + offset;
					sinkPtr->processRetreivedData(areaPtr, dataPtr, size, range);
					});
			}
		}

		void IFrameExecutor::runUpdate(const command::OpUpdate& op)
		{
			auto destObject = objects()->resolveStatic(op.id, ObjectType::Unknown);
			DEBUG_CHECK_RETURN_EX(destObject, "Destination object lost before command buffer was run (waited more than one frame for submission)");

			auto* destCopiable = destObject->toCopiable();
			DEBUG_CHECK_RETURN_EX(destCopiable, "Destination object is not copiable");

			destCopiable->updateFromDynamicData(op.dataBlockPtr, op.dataBlockSize, op.range);
		}

		void IFrameExecutor::runCopy(const command::OpCopy& op)
		{
			auto* source = objects()->resolveStatic(op.src, ObjectType::Unknown);
			auto* target = objects()->resolveStatic(op.dest, ObjectType::Unknown);
			DEBUG_CHECK_RETURN_EX(source && target, "Objects lost");

			auto* sourceCopiable = source->toCopiable();
			auto* targetCopiable = target->toCopiable();
			DEBUG_CHECK_RETURN_EX(sourceCopiable && targetCopiable, "Objects not copiable");

			if (sourceCopiable->objectType() == ObjectType::Image)
			{
				auto sourceImage = static_cast<IBaseImage*>(sourceCopiable);
				targetCopiable->copyFromImage(sourceImage, op.srcRange, op.destRange);
			}
			else if (sourceCopiable->objectType() == ObjectType::Buffer)
			{
				auto sourceBuffer = static_cast<IBaseBuffer*>(sourceCopiable);
				targetCopiable->copyFromBuffer(sourceBuffer, op.srcRange, op.destRange);
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
			ASSERT_EX(op.viewportCount >= 1 && op.viewportCount <= 16, "Invalid viewport count");
			ASSERT_EX(!m_pass.passOp, "Theres already an active pass");
			ASSERT_EX(!m_activePassLayout, "Theres already an active pass");

			// determine rendering area size as we go
			memzero(&m_pass, sizeof(m_pass));

			// set active pass
			m_pass.passOp = &op;
			m_pass.viewportCount = op.viewportCount;

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

			// remember the draw area - this is out initial viewport and scissor rect
			ASSERT(!op.renderArea.empty());
			m_pass.area = op.renderArea;

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
		}

		void IFrameExecutor::runEndPass(const command::OpEndPass& op)
		{
			DEBUG_CHECK_EX(m_pass.passOp, "No pass active");

			m_pass = PassState();
			m_activePassLayout = nullptr;
		}

		//--

    } // api
} // rendering
