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
#include "nullApiVertexLayout.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//--

VertexBindingLayout::VertexBindingLayout(Thread* owner, const Array<ShaderVertexStreamMetadata>& streams)
	: IBaseVertexBindingLayout(owner->objectCache(), streams)
{}

VertexBindingLayout::~VertexBindingLayout()
{}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
