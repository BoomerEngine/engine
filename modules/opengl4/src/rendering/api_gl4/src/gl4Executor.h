/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#pragma once

#include "rendering/api_common/include/apiExecution.h"
#include "gl4StateCache.h"

namespace rendering
{
    namespace api
    {
        namespace gl4
        {

            //---

            /// state tracker for the executed command buffer
            class FrameExecutor : public IFrameExecutor
            {
            public:
				FrameExecutor(Thread* thread, PerformanceStats* stats, const base::Array<GLuint>& glUniformBufferTable);
                ~FrameExecutor();

				INLINE ObjectCache* cache() const { return (ObjectCache*) IFrameExecutor::cache(); }

			private:
				virtual void runBeginPass(const command::OpBeginPass& op) override final;
				virtual void runEndPass(const command::OpEndPass& op) override final;
				virtual void runBeginBlock(const command::OpBeginBlock &) override final;
				virtual void runEndBlock(const command::OpEndBlock &) override final;
				virtual void runResolve(const command::OpResolve &) override final;
				virtual void runClearFrameBuffer(const command::OpClearFrameBuffer&) override final;
				virtual void runClearPassRenderTarget(const command::OpClearPassRenderTarget &) override final;
				virtual void runClearPassDepthStencil(const command::OpClearPassDepthStencil &) override final;
				virtual void runClearRenderTarget(const command::OpClearRenderTarget &) override final;
				virtual void runClearDepthStencil(const command::OpClearDepthStencil &) override final;
				virtual void runClearBuffer(const command::OpClearBuffer &) override final;
				virtual void runClearImage(const command::OpClearImage &) override final;

				virtual void runResourceLayoutBarrier(const command::OpResourceLayoutBarrier&) override final;
				virtual void runUAVBarrier(const command::OpUAVBarrier&) override final;

				virtual void runDraw(const command::OpDraw &) override final;
				virtual void runDrawIndexed(const command::OpDrawIndexed &) override final;
				virtual void runDispatch(const command::OpDispatch &) override final;
				virtual void runDrawIndirect(const command::OpDrawIndirect&) override final;
				virtual void runDrawIndexedIndirect(const command::OpDrawIndexedIndirect&) override final;
				virtual void runDispatchIndirect(const command::OpDispatchIndirect&) override final;

				virtual void runSetViewportRect(const command::OpSetViewportRect& op) override final;
				virtual void runSetScissorRect(const command::OpSetScissorRect& op) override final;
				virtual void runSetBlendColor(const command::OpSetBlendColor& op) override final;
				virtual void runSetLineWidth(const command::OpSetLineWidth& op) override final;
				virtual void runSetDepthClip(const command::OpSetDepthClip& op) override final;
				virtual void runSetStencilReference(const command::OpSetStencilReference& op) override final;

				//---

				const base::Array<GLuint>& m_glUniformBuffers;

				std::atomic<uint32_t> m_nextDebugMessageID = 1;

				StateResources m_currentResources;
				StateValues m_currentRenderState;
				StateMask m_drawRenderStateMask;
				StateMask m_passRenderStateMask;

				GLuint m_glActiveProgram = 0;

				GLuint m_glIndirectBuffer = 0;

				VertexBindingLayout* m_activeVertexLayout = nullptr;

				ResolvedBufferView resolveGeometryBufferView(const rendering::ObjectID& id, uint32_t offset);
				ResolvedBufferView resolveUntypedBufferView(const rendering::ObjectID& viewId);
				ResolvedFormatedView resolveTypedBufferView(const rendering::ObjectID& viewId);
				ResolvedImageView resolveSampledImageView(const rendering::ObjectID& viewID);
				ResolvedImageView resolveWritableImageView(const rendering::ObjectID& viewID);
				ResolvedImageView resolveReadOnlyImageView(const rendering::ObjectID& viewID);

				GLuint resolveSampler(const rendering::ObjectID& id);

				bool prepareDraw(GraphicsPipeline* pso, bool usesIndices);
				bool prepareDispatch(ComputePipeline* pso);
				bool prepareIndirectBuffer(ObjectID indirectBufferId);

				void applyIndexData();
				void applyVertexData(VertexBindingLayout* layout);
				void applyDescriptors(DescriptorBindingLayout* layout);
				
				void flushDrawRenderStates();
				void resetPassRenderStates();

				bool resolveFrameBufferRenderTarget(const FrameBufferAttachmentInfo& fb, ResolvedImageView& outTarget) const;
				bool resolveFrameBufferRenderTargets(const FrameBuffer& fb, FrameBufferTargets& outTargets) const;

				void resetViewport(const FrameBuffer& fb, const base::Rect& rect);
            };

			//---

        } // exec
    } // gl4
} // rendering

