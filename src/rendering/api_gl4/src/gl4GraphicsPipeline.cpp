/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4ObjectCache.h"
#include "gl4Shaders.h"
#include "gl4GraphicsPipeline.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::gl4)

//--

GraphicsPipeline::GraphicsPipeline(Thread* owner, const Shaders* shaders, const GraphicsRenderStatesSetup& mergedRenderStates)
	: IBaseGraphicsPipeline(owner, shaders, mergedRenderStates)
{
	m_staticRenderStates.apply(mergedRenderStates, m_staticRenderStateMask);
}

GraphicsPipeline::~GraphicsPipeline()
{}

bool GraphicsPipeline::apply(GLuint& glActiveProgram)
{
	if (GLuint glProgram = shaders()->object())
	{
		if (glProgram != glActiveProgram)
		{
			GL_PROTECT(glBindProgramPipeline(glProgram));
			glActiveProgram = glProgram;
		}

		return true;
	}

	return false;
}

//--

END_BOOMER_NAMESPACE(rendering::api::gl4)