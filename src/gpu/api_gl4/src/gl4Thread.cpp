/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"

#include "gl4Device.h"
#include "gl4Thread.h"
#include "gl4ObjectCache.h"
#include "gl4Executor.h"
#include "gl4Shaders.h"
#include "gl4Sampler.h"
#include "gl4Image.h"
#include "gl4Buffer.h"
#include "gl4UniformPool.h"

#include "gpu/api_common/include/apiObjectRegistry.h"
#include "gpu/api_common/include/apiSwapchain.h"
#include "core/app/include/commandline.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)


//--

ConfigProperty<float> cvFakeGPUWorkTime("NULL", "FakeGPUWorkTime", 10);
ConfigProperty<bool> cvEnableDebugOutput("GL4", "EnableDebugOutput", true);

//--

namespace prv
{

	void GLDebugPrint(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		if (id == 131222)
			return;
		if (id == 131154)
			return;

		// skip over debug info
		if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
			return;

		// skip over if disabled
		if (!cvEnableDebugOutput.get())
			return;

		// determine the source name
		const char* sourceName = "UnknownSource";
		switch (source) {
		case GL_DEBUG_SOURCE_API:
			sourceName = "API";
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			sourceName = "WindowSystem";
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			sourceName = "ShaderCompiler";
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			sourceName = "ThirdParty";
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			sourceName = "App";
			break;
		case GL_DEBUG_SOURCE_OTHER:
			sourceName = "Other";
			break;
		}

		// determine message type
		const char* typeName = "UnknownType";
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR: typeName = "Error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeName = "DeprectedBehavior"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeName = "UndefinedBehavior"; break;
		case GL_DEBUG_TYPE_PORTABILITY: typeName = "PortabilityIssues"; break;
		case GL_DEBUG_TYPE_PERFORMANCE: typeName = "PerformanceIssue"; break;
		case GL_DEBUG_TYPE_MARKER: typeName = "Marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP: typeName = "PushGroup"; break;
		case GL_DEBUG_TYPE_POP_GROUP: typeName = "PopGroup"; break;
		case GL_DEBUG_TYPE_OTHER: typeName = "Other"; break;
		}

		// determine severity
		const char* severityName = "UnknownSeverity";
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH: severityName = "High"; break;
		case GL_DEBUG_SEVERITY_MEDIUM: severityName = "Medium"; break;
		case GL_DEBUG_SEVERITY_LOW: severityName = "Low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: severityName = "Info"; break;
		}

		// print
		if (type == GL_DEBUG_TYPE_ERROR)
		{
			TRACE_ERROR("{}({}) from {}: ({}) {}", typeName, severityName, sourceName, id, message);
		}
		else
		{
			TRACE_INFO("{}({}) from {}: ({}) {}", typeName, severityName, sourceName, id, message);
		}
	}

} // prv

//--

const char* GetGLErrorDesc(GLuint result)
{
	switch (result)
	{
	case GL_NO_ERROR: return "GL_NO_ERROR";
	case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
	case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
	case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
	case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
	case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
	case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
	case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
	}

	return "Unknown";
}

void ValidateResult(const char* testExpr, uint32_t line, const char* file)
{
#ifndef BUILD_RELEASE
	auto result = glGetError();
	if (result != GL_NO_ERROR)
	{
		logging::Log::Print(logging::OutputLevel::Error, file, line, "OpenGL", "",
			TempString("Function failed with error: {}", GetGLErrorDesc(result)));
	}

#ifdef BUILD_DEBUG
	DEBUG_CHECK_EX(result == GL_NO_ERROR, TempString("Runtime API error: {} at {}", GetGLErrorDesc(result), testExpr));
#endif
#endif
}

//--

Thread::Thread(Device* drv, WindowManager* windows)
	: IBaseThread(drv, windows)
{
}

Thread::~Thread()
{

}

bool Thread::threadStartup(const app::CommandLine& cmdLine, DeviceCaps& outCaps)
{
	// initialize the extension library
	auto ret = glewInit();
	if (GLEW_OK != ret)
	{
		TRACE_ERROR("Failed to initialize GLEW, error: {}", glewGetErrorString(ret));
		return false;
	}

	// print OpenGL information
	TRACE_INFO("Vendor '{}'", (const char*)glGetString(GL_VENDOR));
	TRACE_INFO("Renderer '{}'", (const char*)glGetString(GL_RENDERER));
	TRACE_INFO("Version '{}'", (const char*)glGetString(GL_VERSION));
	TRACE_INFO("GLEW version: {}", (const char*)glewGetString(GLEW_VERSION));
	TRACE_INFO("GLSL version: '{}'", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
	//TRACE_INFO("GLSL extensions '{}'", glGetEx(GL_EXTENENSIONS));

	// get version
	GLint majorVersion = 0;
	GLint minorVersion = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
	glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
	TRACE_INFO("GL version: {}.{}", majorVersion, minorVersion);

	// enable debug output
#ifdef BUILD_RELEASE
	auto enableDebugging = cmdLine.singleValueBool("glDebug", false);
#else
	auto enableDebugging = cmdLine.singleValueBool("glDebug", true);
#endif

	// initialize debugging
	if (enableDebugging)
	{
		GL_PROTECT(glEnable(GL_DEBUG_OUTPUT));
		GL_PROTECT(glDebugMessageCallback(&prv::GLDebugPrint, nullptr));
		TRACE_INFO("Installed OpenGL debug hook");
	}

	// create the pool for uniform values
	m_uniformPool = new UniformBufferPool();
	m_constantBufferAlignment = m_uniformPool->bufferAlignment();
	m_constantBufferSize = m_uniformPool->bufferSize();

	return IBaseThread::threadStartup(cmdLine, outCaps);
}

void Thread::threadFinish()
{
	delete m_uniformPool;
	m_uniformPool = nullptr;

	IBaseThread::threadFinish();
}

//--

void Thread::syncGPU_Thread()
{
	PC_SCOPE_LVL0(SyncGPU);

	ScopeTimer timer;

	auto glFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

	glFlush();

	while (1)
	{
		auto ret = glClientWaitSync(glFence, GL_SYNC_FLUSH_COMMANDS_BIT, 1000);
		if (ret != GL_TIMEOUT_EXPIRED)
			break;
	}

	glDeleteSync(glFence);

	TRACE_SPAM("GPU sync: {}", timer);
}

void UploadConstantBuffers(Thread* thread, const FrameExecutionData& data, Array<GLuint>& outFrameUniformBuffers)
{
	PC_SCOPE_LVL1(UploadConstantBuffers);

	const auto numBuffersNeeded = data.m_constantBuffers.size();

	InplaceArray<uint32_t, 64> maxDirtyRange;
	Array<UniformBuffer> uniformBuffers;
	uniformBuffers.reserve(numBuffersNeeded);
	maxDirtyRange.reserve(numBuffersNeeded);
	outFrameUniformBuffers.reserve(numBuffersNeeded);

	for (auto& cbEntry : data.m_constantBuffers)
	{
		auto bufer = thread->uniformPool()->allocateBuffer();
		cbEntry.resource.handle = bufer.glBuffer;

		outFrameUniformBuffers.pushBack(bufer.glBuffer);
		uniformBuffers.pushBack(bufer);
		maxDirtyRange.pushBack(0);
	}

	for (const auto& copy : data.m_constantBufferCopies)
	{
		auto* targetPtr = uniformBuffers[copy.bufferIndex].memoryPtr + copy.bufferOffset;
		memcpy(targetPtr, copy.srcData, copy.srcDataSize);

		maxDirtyRange[copy.bufferIndex] = std::max<uint32_t>(maxDirtyRange[copy.bufferIndex], copy.bufferOffset + copy.srcDataSize);
	}

	for (uint32_t i = 0; i < numBuffersNeeded; ++i)
	{
		const auto dirtyRange = maxDirtyRange[i];
		GL_PROTECT(glFlushMappedNamedBufferRange(uniformBuffers[i].glBuffer, 0, dirtyRange));
	}

	thread->registerCompletionCallback(DeviceCompletionType::GPUFrameFinished, 
		IDeviceCompletionCallback::CreateFunctionCallback([thread, uniformBuffers]()
		{
				for (auto buffer : uniformBuffers)
					thread->uniformPool()->returnToPool(buffer);
		}));
}
			
void Thread::execute_Thread(uint64_t frameIndex, PerformanceStats& stats, CommandBuffer* masterCommandBuffer, const FrameExecutionData& data)
{
	InplaceArray<GLuint, 128> frameUniformBuffers;
	UploadConstantBuffers(this, data, frameUniformBuffers);

	FrameExecutor executor(this, &stats, frameUniformBuffers);
	executor.execute(masterCommandBuffer);
}
			
void Thread::insertGpuFrameFence_Thread(uint64_t frameIndex)
{
	auto lock = CreateLock(m_fencesLock);
	ASSERT_EX(frameIndex > m_fencesLastFrame, "Unexpected frame index, no fence will be added");

	FrameFence fence;
	fence.frameIndex = frameIndex;
	fence.glFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	ASSERT_EX(fence.glFence != nullptr, "Fence not created");

	m_fences.push(fence);
	m_fencesLastFrame = frameIndex;
}

bool Thread::checkGpuFrameFence_Thread(uint64_t& outCompletedFrameIndex)
{
	auto lock = CreateLock(m_fencesLock);

	if (!m_fences.empty())
	{	
		while (m_fences.size() < 2)
		{
			auto& topFence = m_fences.top();

			auto ret = glClientWaitSync(topFence.glFence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
			if (ret != GL_TIMEOUT_EXPIRED)
			{
				if (ret != GL_CONDITION_SATISFIED && ret != GL_ALREADY_SIGNALED)
				{
					auto error = glGetError();
					TRACE_ERROR("rame fence failed with return code {} and error code {}", ret, error);
				}

				outCompletedFrameIndex = topFence.frameIndex;
				glDeleteSync(topFence.glFence);
				m_fences.pop();
				return true;
			}
		}
	}

	return false;
}

IBaseBuffer* Thread::createOptimalBuffer(const BufferCreationInfo& info, const ISourceDataProvider* sourceData)
{
	return new Buffer(this, info, sourceData);
}

IBaseImage* Thread::createOptimalImage(const ImageCreationInfo& info, const ISourceDataProvider* sourceData)
{
	return new Image(this, info, sourceData);
}

IBaseSampler* Thread::createOptimalSampler(const SamplerState& state)
{
	return new Sampler(this, state);
}

IBaseShaders* Thread::createOptimalShaders(const ShaderData* data)
{
	return new Shaders(this, data);
}
			
ObjectRegistry* Thread::createOptimalObjectRegistry(const app::CommandLine& cmdLine)
{
	return new ObjectRegistry(this);
}

IBaseObjectCache* Thread::createOptimalObjectCache(const app::CommandLine& cmdLine)
{
	return new ObjectCache(this);
}

//--
	
END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
