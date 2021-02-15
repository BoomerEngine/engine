/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#include "build.h"
#include "nullApiExecutor.h"
#include "nullApiThread.h"

#include "rendering/api_common/include/apiExecution.h"
#include "rendering/device/include/renderingDeviceApi.h"

namespace rendering
{
    namespace api
    {
        namespace nul
        {

			//--

			FrameExecutor::FrameExecutor(Thread* thread, PerformanceStats* stats)
				: IFrameExecutor(thread, stats)
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

			void FrameExecutor::runClearFrameBuffer(const command::OpClearFrameBuffer& op)
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

			void FrameExecutor::runClearImage(const command::OpClearImage&)
			{
			}

			void FrameExecutor::runClearBuffer(const command::OpClearBuffer&)
			{
			}

			void FrameExecutor::runClearStructuredBuffer(const command::OpClearStructuredBuffer&)
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

			void FrameExecutor::runDrawIndirect(const command::OpDrawIndirect& op)
			{

			}

			void FrameExecutor::runDrawIndexedIndirect(const command::OpDrawIndexedIndirect& op)
			{

			}

			void FrameExecutor::runDispatchIndirect(const command::OpDispatchIndirect& op)
			{

			}

			void FrameExecutor::runResourceLayoutBarrier(const command::OpResourceLayoutBarrier& op)
			{
			}

			void FrameExecutor::runUAVBarrier(const command::OpUAVBarrier& op)
			{
			}

			void FrameExecutor::runDownload(const command::OpDownload& op)
			{
			}

			void FrameExecutor::runCopyRenderTarget(const command::OpCopyRenderTarget& op)
			{

			}

			void FrameExecutor::runSetViewportRect(const command::OpSetViewportRect& op)
			{

			}

			void FrameExecutor::runSetScissorRect(const command::OpSetScissorRect& op)
			{

			}

			void FrameExecutor::runSetBlendColor(const command::OpSetBlendColor& op)
			{

			}

			void FrameExecutor::runSetLineWidth(const command::OpSetLineWidth& op)
			{

			}

			void FrameExecutor::runSetDepthClip(const command::OpSetDepthClip& op)
			{

			}

			void FrameExecutor::runSetStencilReference(const command::OpSetStencilReference& op)
			{

			}

			//--

        } // exec
    } // gl4
} // rendering
