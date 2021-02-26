/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/api_common/include/apiBackgroundJobs.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//---

class ShaderCompilationJob : public IBaseBackgroundJob
{
public:
	ShaderCompilationJob(boomer::Buffer data, const ShaderMetadata* metadata);
	virtual ~ShaderCompilationJob();

	GLuint extractCompiledProgram();

	virtual void print(IFormatStream& f) const override final;
	virtual void process(volatile bool* vCancelFlag) override final;

private:
	boomer::Buffer m_data;
	ShaderMetadataPtr m_metadata;

	GLuint m_glProgram = 0;
};

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)

