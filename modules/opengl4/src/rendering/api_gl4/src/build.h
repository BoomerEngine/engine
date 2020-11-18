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
} // rendering

#ifndef BUILD_RELEASE
    #define GL_PROTECT(expr) { expr; gl4::ValidateResult(#expr, __LINE__, __FILE__); }
#else
    #define GL_PROTECT(expr) { expr; }
#endif

namespace rendering
{
    namespace gl4
    {

        //--

        enum class ObjectType : uint8_t
        {
            Invalid,
            Buffer,
            Image,
            Sampler,
            Shaders,
            Output,
        };

        enum class TempBufferType : uint8_t
        {
            Constants,
            Storage,
            Geometry,
            Staging,
        };

        //--

        class Device;
        class DeviceThread;

        class Frame;

        class TempBuffer;
        class TempBufferPool;

        class Object;
        class ObjectCache;
        class ObjectRegistry;
        class ObjectRegistryProxy;

        class Buffer;
        class Image;
        class Output;
        class Shaders;
        class Sampler;

        struct ResolvedImageView;
        struct ResolvedFormatedView;
        struct ResolvedBufferView;
        struct ResolvedParameterBindingState;
        struct ResolvedVertexBindingState;

        typedef std::function<void(void)> FrameCompletionCallback;

        //--

    } // gl4
} // rendering
