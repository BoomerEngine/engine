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
    namespace gl4
    {

        /// validate GL API result, prints an error message if something goes wrong
        /// we can enable more strict modes as well via config
        extern RENDERING_API_GL4_API void ValidateResult(const char* testExpr, uint32_t line, const char* file);

    } // gl4
} // driver

#ifndef BUILD_RELEASE
    #define GL_PROTECT(expr) { expr; gl4::ValidateResult(#expr, __LINE__, __FILE__); }
#else
    #define GL_PROTECT(expr) { expr; }
#endif

#include "glObject.h"

namespace rendering
{
    namespace gl4
    {

        class Driver;
        class Swapchain;

        class Buffer;
        class ParameterLayout;
        class SequenceFrame;

        typedef std::function<void(void)> SequenceCompletionCallback;

        extern base::mem::PoolID POOL_GL_STATIC_TEXTURES;
        extern base::mem::PoolID POOL_GL_VERTEX_BUFFER;
        extern base::mem::PoolID POOL_GL_INDEX_BUFFER;
        extern base::mem::PoolID POOL_GL_CONSTANT_BUFFER;
        extern base::mem::PoolID POOL_GL_STORAGE_BUFFER;
        extern base::mem::PoolID POOL_GL_INDIRECT_BUFFER;
        extern base::mem::PoolID POOL_GL_RENDER_TARGETS;
        extern base::mem::PoolID POOL_GL_FRAMEBUFFERS;
        extern base::mem::PoolID POOL_GL_SAMPLERS;
        extern base::mem::PoolID POOL_GL_SHADERS;
        extern base::mem::PoolID POOL_GL_PROGRAMS;

    } // gl4
} // driver

namespace base
{
    namespace image
    {
        class PixelRange;
    } // image
} // base

