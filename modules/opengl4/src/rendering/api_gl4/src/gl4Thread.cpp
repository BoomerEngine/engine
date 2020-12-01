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
#include "gl4FrameFence.h"
#include "gl4CopyQueue.h"
#include "gl4CopyPool.h"
#include "gl4ObjectCache.h"
#include "gl4TransientBuffer.h"
#include "gl4Executor.h"
#include "gl4GraphicsPassLayout.h"
#include "gl4Shaders.h"
#include "gl4Sampler.h"
#include "gl4Image.h"
#include "gl4Buffer.h"

#include "rendering/api_common/include/apiObjectRegistry.h"
#include "rendering/api_common/include/apiSwapchain.h"
#include "base/app/include/commandline.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

	        //--

		    base::ConfigProperty<float> cvFakeGPUWorkTime("NULL", "FakeGPUWorkTime", 10);
			base::ConfigProperty<bool> cvEnableDebugOutput("GL4", "EnableDebugOutput", true);

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
					TRACE_ERROR("Runtime API error: {}", GetGLErrorDesc(result));
					TRACE_ERROR("{}({}): GL: Failed expression: {}", file, line, testExpr);
				}

#ifdef BUILD_DEBUG
				DEBUG_CHECK_EX(result == GL_NO_ERROR, base::TempString("Runtime API error: {} at {}", GetGLErrorDesc(result), testExpr));
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

			bool Thread::threadStartup(const base::app::CommandLine& cmdLine)
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

				return IBaseThread::threadStartup(cmdLine);
			}

			void Thread::threadFinish()
			{
				IBaseThread::threadFinish();
			}

			//--

			void Thread::syncGPU_Thread()
			{
				PC_SCOPE_LVL0(SyncGPU);

				base::ScopeTimer timer;

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

			void Thread::execute_Thread(Frame& frame, PerformanceStats& stats, command::CommandBuffer* masterCommandBuffer, RuntimeDataAllocations& data)
			{
				FrameExecutor executor(this, &frame, &stats);
				executor.prepare(data);
				executor.execute(masterCommandBuffer);
			}
			
			IBaseBuffer* Thread::createOptimalBuffer(const BufferCreationInfo& info)
			{
				return new Buffer(this, info);
			}

			IBaseImage* Thread::createOptimalImage(const ImageCreationInfo& info)
			{
				return new Image(this, info);
			}

			IBaseSampler* Thread::createOptimalSampler(const SamplerState& state)
			{
				return new Sampler(this, state);
			}

			IBaseShaders* Thread::createOptimalShaders(const ShaderData* data)
			{
				return new Shaders(this, data);
			}

			IBaseGraphicsPassLayout* Thread::createOptimalPassLayout(const GraphicsPassLayoutSetup& info)
			{
				return new GraphicsPassLayout(this, info);
			}

			IBaseFrameFence* Thread::createOptimalFrameFence()
			{
				return new FrameFence();
			}

			//--

			IBaseStagingPool* Thread::createOptimalStagingPool(uint32_t size, uint32_t pageSize, const base::app::CommandLine& cmdLine)
			{
				return new CopyPool(size, pageSize);
			}

			IBaseCopyQueue* Thread::createOptimalCopyQueue(const base::app::CommandLine& cmdLine)
			{
				auto* pool = static_cast<CopyPool*>(copyPool());
				return new CopyQueue(this, pool, objectRegistry());
			}

			ObjectRegistry* Thread::createOptimalObjectRegistry(const base::app::CommandLine& cmdLine)
			{
				return new ObjectRegistry(this);
			}

			IBaseObjectCache* Thread::createOptimalObjectCache(const base::app::CommandLine& cmdLine)
			{
				return new ObjectCache(this);
			}

			IBaseTransientBufferPool* Thread::createOptimalTransientStagingPool(const base::app::CommandLine& cmdLine)
			{
				return new TransientBufferPool(this, TransientBufferType::Staging);
			}

			IBaseTransientBufferPool* Thread::createOptimalTransientConstantPool(const base::app::CommandLine& cmdLine)
			{
				return new TransientBufferPool(this, TransientBufferType::Constants);
			}

			//--
	
		} // gl4
    } // api
} // rendering
