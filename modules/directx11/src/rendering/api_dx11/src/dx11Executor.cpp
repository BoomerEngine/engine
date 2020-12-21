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

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

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

			void FrameExecutor::runBeginBlock(const command::OpBeginBlock& op)
			{
				
			}

			void FrameExecutor::runEndBlock(const command::OpEndBlock& op)
			{
			}

			void FrameExecutor::runResolve(const command::OpResolve& op)
			{
			}

			void FrameExecutor::runClearPassRenderTarget(const command::OpClearPassRenderTarget& op)
			{
				ASSERT(m_frameBufferState.dxColorRTV[op.index]);
				m_dxDeviceContext->ClearRenderTargetView(m_frameBufferState.dxColorRTV[op.index], op.color);
			}

			void FrameExecutor::runClearPassDepthStencil(const command::OpClearPassDepthStencil& op)
			{
				ASSERT(m_frameBufferState.dxDepthRTV);
				UINT clearFlags = 0;
				if (op.clearFlags & 1)
					clearFlags |= D3D11_CLEAR_DEPTH;
				if (op.clearFlags & 2)
					clearFlags |= D3D11_CLEAR_STENCIL;
				m_dxDeviceContext->ClearDepthStencilView(m_frameBufferState.dxDepthRTV, clearFlags, op.depthValue, op.stencilValue);
			}

			void FrameExecutor::runClearRenderTarget(const command::OpClearRenderTarget& op)
			{
			}

			void FrameExecutor::runClearDepthStencil(const command::OpClearDepthStencil& op)
			{
			}

			void FrameExecutor::runClearImage(const command::OpClearImage& op)
			{
			}

			void FrameExecutor::runClearBuffer(const command::OpClearBuffer& op)
			{

			}

			void FrameExecutor::runDraw(const command::OpDraw& op)
			{
			}

			void FrameExecutor::runDrawIndexed(const command::OpDrawIndexed& op)
			{
			}

			void FrameExecutor::runDispatch(const command::OpDispatch& op)
			{
			}

			void FrameExecutor::runResourceLayoutBarrier(const command::OpResourceLayoutBarrier& op)
			{
				// not needed
			}

			void FrameExecutor::runUAVBarrier(const command::OpUAVBarrier& op)
			{
				// not needed
			}

			//--

			void FrameExecutor::runSetViewportRect(const command::OpSetViewportRect& op)
			{
				ASSERT(op.viewportIndex < m_frameBufferState.numEnabledViewports);

				auto& state = m_frameBufferState.viewports[op.viewportIndex];
				state.TopLeftX = op.rect.min.x;
				state.TopLeftY = op.rect.min.y;
				state.Width = op.rect.width();
				state.Height = op.rect.height();

				m_dxDeviceContext->RSSetViewports(m_frameBufferState.numEnabledViewports, m_frameBufferState.viewports);
			}

			void FrameExecutor::runSetScissorRect(const command::OpSetScissorRect& op)
			{
				ASSERT(op.viewportIndex < m_frameBufferState.numEnabledViewports);
				auto& state = m_frameBufferState.scissor[op.viewportIndex];
				state.left = op.rect.min.x;
				state.top = op.rect.min.y;
				state.right = op.rect.max.x;
				state.bottom = op.rect.max.y;

				m_dxDeviceContext->RSSetScissorRects(m_frameBufferState.numEnabledViewports, m_frameBufferState.scissor);
			}

			void FrameExecutor::runSetBlendColor(const command::OpSetBlendColor& op)
			{
				m_renderStates.blendColor[0] = op.color[0];
				m_renderStates.blendColor[1] = op.color[1];
				m_renderStates.blendColor[2] = op.color[2];
				m_renderStates.blendColor[3] = op.color[3];
				m_dxDeviceContext->OMSetBlendState(m_renderStates.dxBlendState, m_renderStates.blendColor, m_renderStates.sampleMask);
			}

			void FrameExecutor::runSetLineWidth(const command::OpSetLineWidth& op)
			{
				// not supported
			}

			void FrameExecutor::runSetDepthClip(const command::OpSetDepthClip& op)
			{
				
			}

			void FrameExecutor::runSetStencilReference(const command::OpSetStencilReference& op)
			{
				if (m_renderStates.stencilRef != op.back)
				{
					m_renderStates.stencilRef = op.back;
					m_dxDeviceContext->OMSetDepthStencilState(m_renderStates.dxDepthStencilState, m_renderStates.stencilRef);
				}
			}

			//--

        } // exec
    } // gl4
} // rendering
