/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiObjectCache.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::gl4)

//--

class VertexBindingLayout : public IBaseVertexBindingLayout
{
public:
	VertexBindingLayout(Thread* owner, const base::Array<ShaderVertexStreamMetadata>& streams);
	virtual ~VertexBindingLayout();

	GLuint object();

private:
	GLuint m_glVertexLayout = 0;
};

//--

END_BOOMER_NAMESPACE(rendering::api::gl4)