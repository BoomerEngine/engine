/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "public.h"

#include "GL/glew.h"
#include "base/memory/include/poolStats.h"

#if defined(PLATFORM_LINUX)
#include "GL/glxew.h"
#include <GL/glx.h>
#undef None
#undef Always
#undef bool

typedef GLXContext GLContext;

#elif defined(PLATFORM_WINDOWS)
#include <GL/gl.h>
#include <GL/wglew.h>
typedef void* GLContext;
#endif

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			class Device;
			class Thread;
			class CopyQueue;
			class CopyPool;

			class Buffer;
			class Image;
			class Output;
			class Shaders;
			class Sampler;
			class Swapchain;
			class PassLayout;
			class RenderStates;

			//--

			/// validate GL API result, prints an error message if something goes wrong
			/// we can enable more strict modes as well via config
			extern RENDERING_API_GL4_API void ValidateResult(const char* testExpr, uint32_t line, const char* file);

			//--

		} // gl4 
    } // api
} // rendering

#ifndef BUILD_RELEASE
#define GL_PROTECT(expr) { expr; rendering::api::gl4::ValidateResult(#expr, __LINE__, __FILE__); }
#else
#define GL_PROTECT(expr) { expr; }
#endif