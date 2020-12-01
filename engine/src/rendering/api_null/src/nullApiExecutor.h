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
				virtual void runClear(const command::OpClear &) override final;
				virtual void runDownload(const command::OpDownload &) override final;
				virtual void runUpdate(const command::OpUpdate &) override final;
				virtual void runCopy(const command::OpCopy &) override final;
				virtual void runDraw(const command::OpDraw &) override final;
				virtual void runDrawIndexed(const command::OpDrawIndexed &) override final;
				virtual void runDispatch(const command::OpDispatch &) override final;
				virtual void runResourceLayoutBarrier(const command::OpResourceLayoutBarrier &) override final;
				virtual void runUAVBarrier(const command::OpUAVBarrier &) override final;
            };

			//---

        } // exec
    } // gl4
} // rendering

