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

#include "rendering/api_common/include/apiFrame.h"
#include "rendering/api_common/include/apiExecution.h"
#include "rendering/device/include/renderingDeviceApi.h"

namespace rendering
{
    namespace api
    {
        namespace dx11
        {

			//--

			FrameExecutor::FrameExecutor(Thread* thread, Frame* frame, PerformanceStats* stats)
				: IFrameExecutor(thread, frame, stats)
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
			}

			void FrameExecutor::runClearPassDepthStencil(const command::OpClearPassDepthStencil& op)
			{
			}

			void FrameExecutor::runClearRenderTarget(const command::OpClearRenderTarget& op)
			{
			}

			void FrameExecutor::runClearDepthStencil(const command::OpClearDepthStencil& op)
			{
			}

			void FrameExecutor::runClear(const command::OpClear& op)
			{
			}

			void FrameExecutor::runDownload(const command::OpDownload& op)
			{
			}

			void FrameExecutor::runUpdate(const command::OpUpdate& op)
			{
			}

			void FrameExecutor::runCopy(const command::OpCopy& op)
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
			}

			void FrameExecutor::runUAVBarrier(const command::OpUAVBarrier& op)
			{
			}

			//--

			void FrameExecutor::applyDynamicStates(const DynamicRenderStates& states, DynamicRenderStatesDirtyFlags mask)
			{
			}

			//--

        } // exec
    } // gl4
} // rendering
