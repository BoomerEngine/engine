/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#pragma once

#include "rendering/api_common/include/apiExecution.h"

namespace rendering
{
    namespace api
    {
        namespace dx11
        {

            //---

            /// state tracker for the executed command buffer
            class FrameExecutor : public IFrameExecutor
            {
            public:
                FrameExecutor(Thread* thread, Frame* frame, PerformanceStats* stats);
                ~FrameExecutor();

			private:
				virtual void runBeginBlock(const command::OpBeginBlock &) override final;
				virtual void runEndBlock(const command::OpEndBlock &) override final;
				virtual void runResolve(const command::OpResolve &) override final;
				virtual void runClearPassRenderTarget(const command::OpClearPassRenderTarget &) override final;
				virtual void runClearPassDepthStencil(const command::OpClearPassDepthStencil &) override final;
				virtual void runClearRenderTarget(const command::OpClearRenderTarget &) override final;
				virtual void runClearDepthStencil(const command::OpClearDepthStencil &) override final;
				virtual void runClearImage(const command::OpClearImage&) override final;
				virtual void runClearBuffer(const command::OpClearBuffer&) override final;
				virtual void runDownload(const command::OpDownload &) override final;
				virtual void runUpdate(const command::OpUpdate &) override final;
				virtual void runCopy(const command::OpCopy &) override final;
				virtual void runDraw(const command::OpDraw &) override final;
				virtual void runDrawIndexed(const command::OpDrawIndexed &) override final;
				virtual void runDispatch(const command::OpDispatch &) override final;
				virtual void runResourceLayoutBarrier(const command::OpResourceLayoutBarrier &) override final;
				virtual void runUAVBarrier(const command::OpUAVBarrier &) override final;

				virtual void runSetViewportRect(const command::OpSetViewportRect& op) override final;
				virtual void runSetScissorRect(const command::OpSetScissorRect& op) override final;
				virtual void runSetBlendColor(const command::OpSetBlendColor& op) override final;
				virtual void runSetLineWidth(const command::OpSetLineWidth& op) override final;
				virtual void runSetDepthClip(const command::OpSetDepthClip& op) override final;
				virtual void runSetStencilReference(const command::OpSetStencilReference& op) override final;

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

        } // exec
    } // gl4
} // rendering

