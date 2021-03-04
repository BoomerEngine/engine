/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4ShaderCompilationJob.h"
#include "gl4ShaderCompiler.h"

#include "gpu/device/include/shaderStubs.h"
#include "core/object/include/stubLoader.h"
#include "core/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//--
				
ConfigProperty<bool> cvDumpGLSL("Rendering.Shader", "DumpGLSL", true);
ConfigProperty<bool> cvBreakOnCompilationErrors("Rendering.Shader", "BreakOnCompilationErrors", true);

//--

ShaderCompilationJob::ShaderCompilationJob(boomer::Buffer data, const ShaderMetadata* metadata)
	: m_data(data)
	, m_metadata(AddRef(metadata))
{}

ShaderCompilationJob::~ShaderCompilationJob()
{
	if (m_glProgram)
	{
		GL_PROTECT(glDeleteProgramPipelines(1, &m_glProgram));
		m_glProgram = 0;
	}
}

GLuint ShaderCompilationJob::extractCompiledProgram()
{
	auto ret = m_glProgram;
	m_glProgram = 0;
	return ret;
}

void ShaderCompilationJob::print(IFormatStream& f) const
{
	f << "Shader compilation";
}

static const char* TranslateStageName(ShaderStage stage)
{
	switch (stage)
	{
	case ShaderStage::Pixel: return "glsl_fs";
	case ShaderStage::Vertex: return "glsl_vs";
	case ShaderStage::Geometry: return "glsl_gs";
	case ShaderStage::Hull: return "glsl_tcs";
	case ShaderStage::Domain: return "glsl_tes";
	case ShaderStage::Compute: return "glsl_cs";
	case ShaderStage::Task: return "glsl_ts";
	case ShaderStage::Mesh: return "glsl_ms";
	}

	return "glsl";
}

static GLenum TranslateStageType(ShaderStage stage)
{
	switch (stage)
	{
	case ShaderStage::Pixel: return GL_FRAGMENT_SHADER;
	case ShaderStage::Vertex: return GL_VERTEX_SHADER;
	case ShaderStage::Geometry: return GL_GEOMETRY_SHADER;
	case ShaderStage::Hull: return GL_TESS_CONTROL_SHADER;
	case ShaderStage::Domain: return GL_TESS_EVALUATION_SHADER;
	case ShaderStage::Compute: return GL_COMPUTE_SHADER;
	}

	return 0;
}

static GLenum TranslateStageBit(ShaderStage stage)
{
	switch (stage)
	{
	case ShaderStage::Pixel: return GL_FRAGMENT_SHADER_BIT;
	case ShaderStage::Vertex: return GL_VERTEX_SHADER_BIT;
	case ShaderStage::Geometry: return GL_GEOMETRY_SHADER_BIT;
	case ShaderStage::Hull: return GL_TESS_CONTROL_SHADER_BIT;
	case ShaderStage::Domain: return GL_TESS_EVALUATION_SHADER_BIT;
	case ShaderStage::Compute: return GL_COMPUTE_SHADER_BIT;
	}

	return 0;
}

static void PrintNiceErrorMessage(StringView dumpPath, StringView errorText)
{
	InplaceArray<StringView, 64> lines;
	errorText.slice("\n", false, lines);

	for (auto line : lines)
	{
		if (dumpPath && line.beginsWith("0("))
		{
			uint32_t lineNumber = 0;
			const auto numberText = line.afterFirst("0(").beforeFirst(")");
			if (numberText.match(lineNumber) == MatchResult::OK)
			{
				logging::Log::Print(logging::OutputLevel::Error, dumpPath.data(), lineNumber, "", "", TempString("{}", line.afterFirst(")")));
				continue;
			}
		}

		TRACE_ERROR("GLSL compilation error: {}", line);
	}
}

GLuint CompileStage(const shader::StubProgram* program, const shader::StubStage* stage, const ShaderSharedBindings& bindings)
{
	ScopeTimer stageTimer;
	StringBuilder code;

	// TODO: determine supported flags
	ShaderFeatureMask supportedShaderFeatures;

	// generate code
	{
		PC_SCOPE_LVL0(GenerateGLSL);
		ScopeTimer timer;
		ShaderCodePrinter printer(program, stage, bindings, supportedShaderFeatures);
		printer.printCode(code);

		TRACE_INFO("GLSL code for '{}' {} generated in {}", program->depotPath, stage->stage, timer);
	}

	// dump the code
	StringBuf dumpPath;
	if (cvDumpGLSL.get())
	{
		dumpPath = shader::AssembleDumpFilePath(program->depotPath, program->options, TranslateStageName(stage->stage));
		SaveFileFromString(dumpPath, code.view());
		TRACE_INFO("Saved GLSL dump file '{}'", dumpPath);
	}

	// determine what kind of shader to actually assemble
	GLuint shaderID = glCreateShader(TranslateStageType(stage->stage));
	{
		const GLchar* shaderSourceArray[1] = { code.c_str() };
		const GLint shaderSourceLengthArray[1] = { code.view().length() };
		GL_PROTECT(glShaderSource(shaderID, 1, shaderSourceArray, shaderSourceLengthArray));
	}

	// compile the shader
	{
		PC_SCOPE_LVL0(CompileGLSL);

		ScopeTimer timer;
		GL_PROTECT(glCompileShader(shaderID));
		TRACE_INFO("GLSL code for '{}' {} compiled in {}", program->depotPath, stage->stage, timer);
	}

	// check compilation error
	GLint shaderCompiled = 0;;
	GL_PROTECT(glGetShaderiv(shaderID, GL_COMPILE_STATUS, &shaderCompiled));
	if (shaderCompiled != GL_TRUE)
	{
		// get the compilation error
		GLsizei messageSize = 0;
		GLchar message[4096];
		GL_PROTECT(glGetShaderInfoLog(shaderID, sizeof(message), &messageSize, message));
		message[messageSize] = 0;

		// print the error
		PrintNiceErrorMessage(dumpPath, message);

		// stop and allow for debugging
		if (cvBreakOnCompilationErrors.get())
		{
			logging::IErrorHandler::Assert(false, __FILE__, __LINE__, "GLSL compilation error", message, &cvBreakOnCompilationErrors.get());
		}

		// cleanup
		GL_PROTECT(glDeleteShader(shaderID));
		return 0;
	}

	// create shader program from this shader
	GLuint programID = 0;
	GL_PROTECT(programID = glCreateProgram());
	GL_PROTECT(glProgramParameteri(programID, GL_PROGRAM_SEPARABLE, GL_TRUE));
	GL_PROTECT(glProgramParameteri(programID, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE));

	// link shader into program
	GL_PROTECT(glAttachShader(programID, shaderID));
	GL_PROTECT(glLinkProgram(programID));

	// check if the linking worked
	GLint linkStatus = GL_TRUE;
	GL_PROTECT(glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus));
	GL_PROTECT(glDetachShader(programID, shaderID));

	// link failed
	if (linkStatus != GL_TRUE)
	{
		// get linking error
		int bufferSize = 0;
		GL_PROTECT(glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &bufferSize));
		if (bufferSize > 0)
		{
			Array<char> buffer;
			buffer.resize(bufferSize);
			GL_PROTECT(glGetProgramInfoLog(programID, bufferSize, &bufferSize, buffer.typedData()));

			// print the error
			PrintNiceErrorMessage(dumpPath, buffer.typedData());

			// stop and allow for debugging
			if (cvBreakOnCompilationErrors.get())
			{
				logging::IErrorHandler::Assert(false, __FILE__, __LINE__, "shderCompiled != GL_TRUE",
					TempString("GLSL linking error!: {}", buffer.typedData()), &cvBreakOnCompilationErrors.get());
			}

			// cleanup
			GL_PROTECT(glDeleteProgram(programID));
			GL_PROTECT(glDeleteShader(shaderID));
			return 0;
		}
	}

	// validate program
	GL_PROTECT(glValidateProgram(programID));

	// discard shader that is not part of the program
	GL_PROTECT(glDeleteShader(shaderID));
	return programID;
}

void ShaderCompilationJob::process(volatile bool* vCancelFlag)
{
	ScopeTimer timer;

	// TODO: try loading program from cache


	// unpack data
	StubLoader loader(shader::Stub::Factory(), 1, POOL_API_SHADERS);
	const auto* program = (const shader::StubProgram*) loader.unpack(m_data);
	DEBUG_CHECK_RETURN_EX(program, "Failed to unpack shader data");
				
	// prepare shared bindings
	const ShaderSharedBindings bindings(program);

	// create target pipeline
	GLuint pipelineID = 0;
	GL_PROTECT(glCreateProgramPipelines(1, &pipelineID));

	// generate shader code for each stage
	// TODO: this can happen in parallel
	InplaceArray<GLuint, 5> programs;
	for (const auto* stage : program->stages)
	{
		if (auto programID = CompileStage(program, stage, bindings))
		{
			GL_PROTECT(glUseProgramStages(pipelineID, TranslateStageBit(stage->stage), programID));
			programs.pushBack(programID);
		}
		else
		{
			for (auto programID : programs)
				GL_PROTECT(glDeleteProgram(programID));
			GL_PROTECT(glDeleteProgramPipelines(1, &pipelineID));
			return;
		}
	}

	// finished
	m_glProgram = pipelineID;
	TRACE_INFO("Finished GLSL compilation for '{}' in {}", program->depotPath, timer);
}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
