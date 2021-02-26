/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#pragma once

#include "gpu/api_common/include/apiExecution.h"
#include "gl4StateCache.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//---

/// state tracker for the executed command buffer
class FrameExecutor : public IFrameExecutor
{
public:
	FrameExecutor(Thread* thread, PerformanceStats* stats, const Array<GLuint>& glUniformBufferTable);
    ~FrameExecutor();

	INLINE ObjectCache* cache() const { return (ObjectCache*) IFrameExecutor::cache(); }

private:
	virtual void runBeginPass(const OpBeginPass& op) override final;
	virtual void runEndPass(const OpEndPass& op) override final;
	virtual void runBeginBlock(const OpBeginBlock &) override final;
	virtual void runEndBlock(const OpEndBlock &) override final;
	virtual void runResolve(const OpResolve &) override final;
	virtual void runClearFrameBuffer(const OpClearFrameBuffer&) override final;
	virtual void runClearPassRenderTarget(const OpClearPassRenderTarget &) override final;
	virtual void runClearPassDepthStencil(const OpClearPassDepthStencil &) override final;
	virtual void runClearRenderTarget(const OpClearRenderTarget &) override final;
	virtual void runClearDepthStencil(const OpClearDepthStencil &) override final;
	virtual void runClearBuffer(const OpClearBuffer &) override final;
	virtual void runClearStructuredBuffer(const OpClearStructuredBuffer&) override final;
	virtual void runClearImage(const OpClearImage &) override final;
    virtual void runDownload(const OpDownload&) override final;
	virtual void runCopyRenderTarget(const OpCopyRenderTarget&) override final;

	virtual void runResourceLayoutBarrier(const OpResourceLayoutBarrier&) override final;
	virtual void runUAVBarrier(const OpUAVBarrier&) override final;

	virtual void runDraw(const OpDraw &) override final;
	virtual void runDrawIndexed(const OpDrawIndexed &) override final;
	virtual void runDispatch(const OpDispatch &) override final;
	virtual void runDrawIndirect(const OpDrawIndirect&) override final;
	virtual void runDrawIndexedIndirect(const OpDrawIndexedIndirect&) override final;
	virtual void runDispatchIndirect(const OpDispatchIndirect&) override final;

	virtual void runSetViewportRect(const OpSetViewportRect& op) override final;
	virtual void runSetScissorRect(const OpSetScissorRect& op) override final;
	virtual void runSetBlendColor(const OpSetBlendColor& op) override final;
	virtual void runSetLineWidth(const OpSetLineWidth& op) override final;
	virtual void runSetDepthClip(const OpSetDepthClip& op) override final;
	virtual void runSetStencilReference(const OpSetStencilReference& op) override final;

	//---

	const Array<GLuint>& m_glUniformBuffers;

	std::atomic<uint32_t> m_nextDebugMessageID = 1;

	StateResources m_currentResources;
	StateValues m_currentRenderState;
	StateMask m_drawRenderStateMask;
	StateMask m_passRenderStateMask;

	GLuint m_glActiveProgram = 0;

	GLuint m_glIndirectBuffer = 0;

	VertexBindingLayout* m_activeVertexLayout = nullptr;

	ResolvedBufferView resolveGeometryBufferView(const gpu::ObjectID& id, uint32_t offset);
	ResolvedBufferView resolveUntypedBufferView(const gpu::ObjectID& viewId);
	ResolvedFormatedView resolveTypedBufferView(const gpu::ObjectID& viewId);
	ResolvedImageView resolveSampledImageView(const gpu::ObjectID& viewID);
	ResolvedImageView resolveWritableImageView(const gpu::ObjectID& viewID);
	ResolvedImageView resolveReadOnlyImageView(const gpu::ObjectID& viewID);

	GLuint resolveSampler(const gpu::ObjectID& id);

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

	void resetViewport(const FrameBuffer& fb, const Rect& rect);
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
