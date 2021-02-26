/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#pragma once

#include "gpu/api_common/include/apiExecution.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::dx11)

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
	virtual void runClearPassRenderTarget(const OpClearPassRenderTarget &) override final;
	virtual void runClearPassDepthStencil(const OpClearPassDepthStencil &) override final;
	virtual void runClearRenderTarget(const OpClearRenderTarget &) override final;
	virtual void runClearDepthStencil(const OpClearDepthStencil &) override final;
	virtual void runClearImage(const OpClearImage&) override final;
	virtual void runClearBuffer(const OpClearBuffer&) override final;
	virtual void runClearStructuredBuffer(const OpClearStructuredBuffer&) override final;
	virtual void runDraw(const OpDraw &) override final;
	virtual void runDrawIndexed(const OpDrawIndexed &) override final;
	virtual void runDispatch(const OpDispatch &) override final;
	virtual void runResourceLayoutBarrier(const OpResourceLayoutBarrier &) override final;
	virtual void runUAVBarrier(const OpUAVBarrier &) override final;
    virtual void runDownload(const OpDownload&) override final;
	virtual void runCopyRenderTarget(const OpCopyRenderTarget&) override final;

	virtual void runSetViewportRect(const OpSetViewportRect& op) override final;
	virtual void runSetScissorRect(const OpSetScissorRect& op) override final;
	virtual void runSetBlendColor(const OpSetBlendColor& op) override final;
	virtual void runSetLineWidth(const OpSetLineWidth& op) override final;
	virtual void runSetDepthClip(const OpSetDepthClip& op) override final;
	virtual void runSetStencilReference(const OpSetStencilReference& op) override final;

	//--

	ID3D11Device* m_dxDevice = nullptr;
	ID3D11DeviceContext* m_dxDeviceContext = nullptr;

	struct FrameBufferState
	{
		ID3D11DepthStencilView* dxDepthRTV = nullptr;
		ID3D11RenderTargetView* dxColorRTV[8];

		D3D11_VIEWPORT viewports[16];
		D3D11_RECT scissor[16];

		uint8_t numColorTargets = 0;
		uint8_t numEnabledViewports = 0;

		INLINE FrameBufferState()
		{
			memzero(dxColorRTV, sizeof(dxColorRTV));
		}
	};

	struct ActiveRenderStates
	{
		ID3D11BlendState* dxBlendState = nullptr;
		ID3D11DepthStencilState* dxDepthStencilState = nullptr;

		FLOAT blendColor[4];
		UINT sampleMask = 0xFFFFFFFF;
		UINT stencilRef = 0;
	};

	FrameBufferState m_frameBufferState;
	ActiveRenderStates m_renderStates;

	//--
				
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api::dx11)
