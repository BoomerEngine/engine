/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiComputePipeline.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::nul)

//--

ComputePipeline::ComputePipeline(Thread* owner, const Shaders* shaders)
	: IBaseComputePipeline(owner, shaders)
{}

ComputePipeline::~ComputePipeline()
{}

//--

END_BOOMER_NAMESPACE(rendering::api::nul)