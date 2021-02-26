/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "gpu/api_common/include/apiObjectCache.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::nul)

//--

class VertexBindingLayout : public IBaseVertexBindingLayout
{
public:
	VertexBindingLayout(Thread* owner, const Array<ShaderVertexStreamMetadata>& streams);
	virtual ~VertexBindingLayout();
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::nul)
