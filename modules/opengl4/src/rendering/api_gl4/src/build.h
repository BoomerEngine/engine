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
			class UniformBufferPool;
			class ObjectCache;
            class DownloadArea;

			class Buffer;
			class Image;
			class Output;
			class Shaders;
			class Sampler;
			class Swapchain;
			class PassLayout;
			class RenderStates;
			class GraphicsPipeline;
			class ComputePipeline;
			class DescriptorBindingLayout;
			class VertexBindingLayout;

			struct FrameBufferTargets;
			struct FrameBufferTargetInfo;

			//--

			/// validate GL API result, prints an error message if something goes wrong
			/// we can enable more strict modes as well via config
			extern RENDERING_API_GL4_API void ValidateResult(const char* testExpr, uint32_t line, const char* file);

			//--

			struct ResolvedFormatedView
			{
				GLuint glBufferView = 0;
				GLuint glViewFormat = 0;
				uint32_t size = 0;

				INLINE operator bool() const { return (glBufferView != 0); }
				INLINE bool operator==(const ResolvedFormatedView& other) const { return (glBufferView == other.glBufferView) && (glViewFormat == other.glViewFormat); }
				INLINE bool operator!=(const ResolvedFormatedView& other) const { return !operator==(other); }
			};

			//--

			struct ResolvedBufferView
			{
				GLuint glBuffer = 0;
				uint32_t offset = 0;
				uint32_t size = 0;

				INLINE operator bool() const { return glBuffer != 0; }
				INLINE bool operator==(const ResolvedBufferView& other) const { return (glBuffer == other.glBuffer) && (offset == other.offset) && (size == other.size); }
				INLINE bool operator!=(const ResolvedBufferView& other) const { return !operator==(other); }
			};

			//--

			struct ResolvedImageView
			{
				GLuint glImage = 0; // actual image
				GLuint glImageView = 0; // view
				GLenum glViewType = 0; // GL_TEXTURE_2D, etc
				GLenum glInternalFormat = 0;
				uint16_t firstSlice = 0;
				uint16_t numSlices = 0;
				uint8_t firstMip = 0;
				uint8_t numMips = 0;

				INLINE operator bool() const { return glImageView != 0; }
				INLINE bool operator==(const ResolvedImageView& other) const
				{
					return (glImageView == other.glImageView) && (glViewType == other.glViewType) && (glInternalFormat == other.glInternalFormat)
						&& (firstMip == other.firstMip) && (numMips == other.numMips) && (firstSlice == other.firstSlice) && (numMips == other.numMips);
				}
				INLINE bool operator!=(const ResolvedImageView& other) const { return !operator==(other); }
			};

			//--

		} // gl4 
    } // api
} // rendering

#ifndef BUILD_RELEASE
#define GL_PROTECT(expr) { expr; rendering::api::gl4::ValidateResult(#expr, __LINE__, __FILE__); }
#else
#define GL_PROTECT(expr) { expr; }
#endif