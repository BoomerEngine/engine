/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#pragma once

#include "rendering/api_common/include/apiExecution.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::nul)

//---

/// state tracker for the executed command buffer
class FrameExecutor : public IFrameExecutor
{
public:
    FrameExecutor(Thread* thread, PerformanceStats* stats);
    ~FrameExecutor();

private:
	virtual void runBeginBlock(const OpBeginBlock &) override final;
	virtual void runEndBlock(const OpEndBlock &) override final;
	virtual void runResolve(const OpResolve &) override final;
	virtual void runClearFrameBuffer(const OpClearFrameBuffer&) override final;
	virtual void runClearPassRenderTarget(const OpClearPassRenderTarget &) override final;
	virtual void runClearPassDepthStencil(const OpClearPassDepthStencil &) override final;
	virtual void runClearRenderTarget(const OpClearRenderTarget &) override final;
	virtual void runClearDepthStencil(const OpClearDepthStencil &) override final;
	virtual void runClearImage(const OpClearImage &) override final;
	virtual void runClearBuffer(const OpClearBuffer&) override final;
	virtual void runClearStructuredBuffer(const OpClearStructuredBuffer&) override final;
	virtual void runResourceLayoutBarrier(const OpResourceLayoutBarrier &) override final;
	virtual void runUAVBarrier(const OpUAVBarrier &) override final;
	virtual void runDownload(const OpDownload&) override final;
	virtual void runCopyRenderTarget(const OpCopyRenderTarget&) override final;

	virtual void runDraw(const OpDraw&) override final;
	virtual void runDrawIndexed(const OpDrawIndexed&) override final;
	virtual void runDispatch(const OpDispatch&) override final;
	virtual void runDrawIndirect(const OpDrawIndirect&) override final;
	virtual void runDrawIndexedIndirect(const OpDrawIndexedIndirect&) override final;
	virtual void runDispatchIndirect(const OpDispatchIndirect&) override final;

	virtual void runSetViewportRect(const OpSetViewportRect& op) override final;
	virtual void runSetScissorRect(const OpSetScissorRect& op) override final;
	virtual void runSetBlendColor(const OpSetBlendColor& op) override final;
	virtual void runSetLineWidth(const OpSetLineWidth& op) override final;
	virtual void runSetDepthClip(const OpSetDepthClip& op) override final;
	virtual void runSetStencilReference(const OpSetStencilReference& op) override final;
};

//---

BEGIN_BOOMER_NAMESPACE(rendering::api::nul)