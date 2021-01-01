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
        namespace nul
        {

            //---

            /// state tracker for the executed command buffer
            class FrameExecutor : public IFrameExecutor
            {
            public:
                FrameExecutor(Thread* thread, PerformanceStats* stats);
                ~FrameExecutor();

			private:
				virtual void runBeginBlock(const command::OpBeginBlock &) override final;
				virtual void runEndBlock(const command::OpEndBlock &) override final;
				virtual void runResolve(const command::OpResolve &) override final;
				virtual void runClearFrameBuffer(const command::OpClearFrameBuffer&) override final;
				virtual void runClearPassRenderTarget(const command::OpClearPassRenderTarget &) override final;
				virtual void runClearPassDepthStencil(const command::OpClearPassDepthStencil &) override final;
				virtual void runClearRenderTarget(const command::OpClearRenderTarget &) override final;
				virtual void runClearDepthStencil(const command::OpClearDepthStencil &) override final;
				virtual void runClearImage(const command::OpClearImage &) override final;
				virtual void runClearBuffer(const command::OpClearBuffer&) override final;
				virtual void runClearStructuredBuffer(const command::OpClearStructuredBuffer&) override final;				
				virtual void runResourceLayoutBarrier(const command::OpResourceLayoutBarrier &) override final;
				virtual void runUAVBarrier(const command::OpUAVBarrier &) override final;
				virtual void runDownload(const command::OpDownload&) override final;

				virtual void runDraw(const command::OpDraw&) override final;
				virtual void runDrawIndexed(const command::OpDrawIndexed&) override final;
				virtual void runDispatch(const command::OpDispatch&) override final;
				virtual void runDrawIndirect(const command::OpDrawIndirect&) override final;
				virtual void runDrawIndexedIndirect(const command::OpDrawIndexedIndirect&) override final;
				virtual void runDispatchIndirect(const command::OpDispatchIndirect&) override final;

				virtual void runSetViewportRect(const command::OpSetViewportRect& op) override final;
				virtual void runSetScissorRect(const command::OpSetScissorRect& op) override final;
				virtual void runSetBlendColor(const command::OpSetBlendColor& op) override final;
				virtual void runSetLineWidth(const command::OpSetLineWidth& op) override final;
				virtual void runSetDepthClip(const command::OpSetDepthClip& op) override final;
				virtual void runSetStencilReference(const command::OpSetStencilReference& op) override final;
            };

			//---

        } // exec
    } // gl4
} // rendering

