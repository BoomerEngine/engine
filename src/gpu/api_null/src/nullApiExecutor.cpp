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

#include "gpu/api_common/include/apiExecution.h"
#include "gpu/device/include/device.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//--

FrameExecutor::FrameExecutor(Thread* thread, PerformanceStats* stats)
	: IFrameExecutor(thread, stats)
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

void FrameExecutor::runClearFrameBuffer(const OpClearFrameBuffer& op)
{

}

void FrameExecutor::runClearPassRenderTarget(const OpClearPassRenderTarget& op)
{
}

void FrameExecutor::runClearPassDepthStencil(const OpClearPassDepthStencil& op)
{
}

void FrameExecutor::runClearRenderTarget(const OpClearRenderTarget& op)
{
}

void FrameExecutor::runClearDepthStencil(const OpClearDepthStencil& op)
{
}

void FrameExecutor::runClearImage(const OpClearImage&)
{
}

void FrameExecutor::runClearBuffer(const OpClearBuffer&)
{
}

void FrameExecutor::runClearStructuredBuffer(const OpClearStructuredBuffer&)
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

void FrameExecutor::runDrawIndirect(const OpDrawIndirect& op)
{

}

void FrameExecutor::runDrawIndexedIndirect(const OpDrawIndexedIndirect& op)
{

}

void FrameExecutor::runDispatchIndirect(const OpDispatchIndirect& op)
{

}

void FrameExecutor::runResourceLayoutBarrier(const OpResourceLayoutBarrier& op)
{
}

void FrameExecutor::runUAVBarrier(const OpUAVBarrier& op)
{
}

void FrameExecutor::runDownload(const OpDownload& op)
{
}

void FrameExecutor::runCopyRenderTarget(const OpCopyRenderTarget& op)
{

}

void FrameExecutor::runSetViewportRect(const OpSetViewportRect& op)
{

}

void FrameExecutor::runSetScissorRect(const OpSetScissorRect& op)
{

}

void FrameExecutor::runSetBlendColor(const OpSetBlendColor& op)
{

}

void FrameExecutor::runSetLineWidth(const OpSetLineWidth& op)
{

}

void FrameExecutor::runSetDepthClip(const OpSetDepthClip& op)
{

}

void FrameExecutor::runSetStencilReference(const OpSetStencilReference& op)
{

}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
