/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Shaders.h"
#include "gl4ShaderCompilationJob.h"
#include "gl4ComputePipeline.h"
#include "gl4GraphicsPipeline.h"

BEGIN_BOOMER_NAMESPACE(rendering::api::gl4)

//--

Shaders::Shaders(Thread* drv, const ShaderData* data)
	: IBaseShaders(drv, data)
{
	// start compiling the shader as soon as possible
	// NOTE: this may fetch data from the shader cache
	m_compilationJob = base::RefNew<ShaderCompilationJob>(data->data(), data->metadata());
	drv->backgroundQueue()->pushNormalJob(m_compilationJob);
}

Shaders::~Shaders()
{
	if (m_glProgram)
	{
		GL_PROTECT(glDeleteProgramPipelines(1, &m_glProgram));
		m_glProgram = 0;
	}
}

IBaseGraphicsPipeline* Shaders::createGraphicsPipeline_ClientApi(const GraphicsRenderStatesSetup& setup)
{
	DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Vertex), "Shader bundle has no vertex shader", nullptr);
	DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Pixel), "Shader bundle has no pixel shader", nullptr);
	return new GraphicsPipeline(owner(), this, setup);
}

IBaseComputePipeline* Shaders::createComputePipeline_ClientApi()
{
	DEBUG_CHECK_RETURN_EX_V(mask().test(ShaderStage::Compute), "Shader bundle has no compute shader", nullptr);
	return new ComputePipeline(owner(), this);
}

GLuint Shaders::object()
{
	if (m_glProgram == 0 && m_compilationJob)
	{
		m_compilationJob->waitUntilCompleted();
		m_glProgram = m_compilationJob->extractCompiledProgram();

		m_compilationJob.reset();
	}

	return m_glProgram;
}

//--

END_BOOMER_NAMESPACE(rendering::api::gl4)
