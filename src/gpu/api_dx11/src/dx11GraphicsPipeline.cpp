/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects\pipeline #]
***/

#include "build.h"
#include "dx11Thread.h"
#include "dx11ObjectCache.h"
#include "dx11GraphicsPipeline.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::dx11)

//--

GraphicsPipeline::GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsRenderStatesSetup& mergedRenderStates)
	: IBaseGraphicsPipeline(owner, shaders, mergedRenderStates)
{
}

GraphicsPipeline::~GraphicsPipeline()
{}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::dx11)
