/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiThread.h"
#include "nullApiObjectCache.h"
#include "nullApiGraphicsPipeline.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//--

GraphicsPipeline::GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsRenderStatesSetup& mergedRenderStates)
	: IBaseGraphicsPipeline(owner, shaders, mergedRenderStates)
{
}

GraphicsPipeline::~GraphicsPipeline()
{}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
