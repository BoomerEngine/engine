/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#include "build.h"
#include "dx11Executor.h"
#include "dx11Thread.h"

#include "rendering/api_common/include/apiExecution.h"
#include "rendering/device/include/renderingDeviceApi.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::dx11)

//--

FrameExecutor::FrameExecutor(Thread* thread, PerformanceStats* stats)
	: IFrameExecutor(thread, stats)
	, m_dxDevice(thread->device())
	, m_dxDeviceContext(thread->deviceContext())
{
}

FrameExecutor::~FrameExecutor()
{}

//--

void FrameExecutor::runBeginBlock(const OpBeginBlock& op)
{
				
}

void FrameExecutor::runEndBlock(const OpEndBlock& op)
{
}

void FrameExecutor::runResolve(const OpResolve& op)
{
}

void FrameExecutor::runClearPassRenderTarget(const OpClearPassRenderTarget& op)
{
	ASSERT(m_frameBufferState.dxColorRTV[op.index]);
	m_dxDeviceContext->ClearRenderTargetView(m_frameBufferState.dxColorRTV[op.index], op.color);
}

void FrameExecutor::runClearPassDepthStencil(const OpClearPassDepthStencil& op)
{
	ASSERT(m_frameBufferState.dxDepthRTV);
	UINT clearFlags = 0;
	if (op.clearFlags & 1)
		clearFlags |= D3D11_CLEAR_DEPTH;
	if (op.clearFlags & 2)
		clearFlags |= D3D11_CLEAR_STENCIL;
	m_dxDeviceContext->ClearDepthStencilView(m_frameBufferState.dxDepthRTV, clearFlags, op.depthValue, op.stencilValue);
}

void FrameExecutor::runClearRenderTarget(const OpClearRenderTarget& op)
{
}

void FrameExecutor::runClearDepthStencil(const OpClearDepthStencil& op)
{
}

void FrameExecutor::runClearImage(const OpClearImage& op)
{
}

void FrameExecutor::runClearBuffer(const OpClearBuffer& op)
{
}

void FrameExecutor::runClearStructuredBuffer(const OpClearStructuredBuffer& op)
{
}

void FrameExecutor::runDraw(const OpDraw& op)
{
}

void FrameExecutor::runDrawIndexed(const OpDrawIndexed& op)
{
}

void FrameExecutor::runDispatch(const OpDispatch& op)
{
}

void FrameExecutor::runResourceLayoutBarrier(const OpResourceLayoutBarrier& op)
{
	// not needed
}

void FrameExecutor::runUAVBarrier(const OpUAVBarrier& op)
{
	// not needed
}

void FrameExecutor::runDownload(const OpDownload& op)
{

}

void FrameExecutor::runCopyRenderTarget(const OpCopyRenderTarget& op)
{

}

//--

void FrameExecutor::runSetViewportRect(const OpSetViewportRect& op)
{
	ASSERT(op.viewportIndex < m_frameBufferState.numEnabledViewports);

	auto& state = m_frameBufferState.viewports[op.viewportIndex];
	state.TopLeftX = op.rect.min.x;
	state.TopLeftY = op.rect.min.y;
	state.Width = op.rect.width();
	state.Height = op.rect.height();

	m_dxDeviceContext->RSSetViewports(m_frameBufferState.numEnabledViewports, m_frameBufferState.viewports);
}

void FrameExecutor::runSetScissorRect(const OpSetScissorRect& op)
{
	ASSERT(op.viewportIndex < m_frameBufferState.numEnabledViewports);
	auto& state = m_frameBufferState.scissor[op.viewportIndex];
	state.left = op.rect.min.x;
	state.top = op.rect.min.y;
	state.right = op.rect.max.x;
	state.bottom = op.rect.max.y;

	m_dxDeviceContext->RSSetScissorRects(m_frameBufferState.numEnabledViewports, m_frameBufferState.scissor);
}

void FrameExecutor::runSetBlendColor(const OpSetBlendColor& op)
{
	m_renderStates.blendColor[0] = op.color[0];
	m_renderStates.blendColor[1] = op.color[1];
	m_renderStates.blendColor[2] = op.color[2];
	m_renderStates.blendColor[3] = op.color[3];
	m_dxDeviceContext->OMSetBlendState(m_renderStates.dxBlendState, m_renderStates.blendColor, m_renderStates.sampleMask);
}

void FrameExecutor::runSetLineWidth(const OpSetLineWidth& op)
{
	// not supported
}

void FrameExecutor::runSetDepthClip(const OpSetDepthClip& op)
{
				
}

void FrameExecutor::runSetStencilReference(const OpSetStencilReference& op)
{
	if (m_renderStates.stencilRef != op.back)
	{
		m_renderStates.stencilRef = op.back;
		m_dxDeviceContext->OMSetDepthStencilState(m_renderStates.dxDepthStencilState, m_renderStates.stencilRef);
	}
}

//--

END_BOOMER_NAMESPACE(rendering::api::dx11)