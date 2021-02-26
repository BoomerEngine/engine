/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\execution #]
***/

#include "build.h"
#include "gl4Executor.h"
#include "gl4Thread.h"
#include "gl4Image.h"
#include "gl4ObjectCache.h"
#include "gl4Buffer.h"
#include "gl4Utils.h"
#include "gl4ComputePipeline.h"
#include "gl4GraphicsPipeline.h"
#include "gl4DescriptorLayout.h"
#include "gl4VertexLayout.h"
#include "gl4Sampler.h"

#include "gpu/api_common/include/apiExecution.h"
#include "gpu/api_common/include/apiObjectRegistry.h"
#include "gpu/api_common/include/apiOutput.h"
#include "gpu/device/include/renderingDeviceApi.h"
#include "gpu/device/include/renderingDescriptorInfo.h"
#include "gpu/device/include/renderingDescriptor.h"
#include "core/image/include/imageRect.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//--

FrameExecutor::FrameExecutor(Thread* thread, PerformanceStats* stats, const Array<GLuint>& glUniformBufferTable)
	: IFrameExecutor(thread, stats)
	, m_glUniformBuffers(glUniformBufferTable)
{
}

FrameExecutor::~FrameExecutor()
{
}
           
//--

void FrameExecutor::resetViewport(const FrameBuffer& fb, const Rect& rect)
{
	DEBUG_CHECK(m_currentRenderState.common.scissorEnabled == false);
	m_currentRenderState.common.scissorEnabled = false;
	m_drawRenderStateMask |= State_ScissorEnabled;

	GL_PROTECT(glEnable(GL_SCISSOR_TEST));

	for (uint32_t i = 0; i < m_pass.viewportCount; ++i)
	{
		const float x = rect.left();
		//const float y = (int)m_pass.height - rect.top() - rect.height();
		const float y = rect.top();
		const float w = rect.width();
		const float h = rect.height();

		GL_PROTECT(glViewportIndexedf(i, x, y, w, h));
		GL_PROTECT(glDepthRangeIndexed(i, 0, 1));
		GL_PROTECT(glScissorIndexed(i, x, y, w, h));
		//GL_PROTECT(glColorMaski(i, true, true, true, true));
	}

	for (uint32_t i = 0; i < FrameBuffer::MAX_COLOR_TARGETS; ++i)
	{
		if (fb.color[i].viewID && fb.color[i].loadOp == LoadOp::Clear)
		{
			static FastRandState rnd;
			GL_PROTECT(glClearBufferfv(GL_COLOR, i, fb.color[i].clearColorValues));
			//GL_PROTECT(glClearNamedFramebufferfv(m_pass.m_glFrameBuffer, GL_COLOR, i, (GLfloat*)&fb.color[i].clearColorValues));
		}
	}

	if (fb.depth.viewID && fb.depth.loadOp == LoadOp::Clear)
	{
		//GL_PROTECT(glClearNamedFramebufferfi(m_pass.m_glFrameBuffer, GL_DEPTH_STENCIL, 0, fb.depth.clearDepthValue, fb.depth.clearStencilValue));
		GL_PROTECT(glClearBufferfi(GL_DEPTH_STENCIL, 0, fb.depth.clearDepthValue, fb.depth.clearStencilValue));
	}

	GL_PROTECT(glDisable(GL_SCISSOR_TEST));
}

bool FrameExecutor::resolveFrameBufferRenderTarget(const FrameBufferAttachmentInfo& fb, ResolvedImageView& outTarget) const
{
	if (!fb.viewID)
	{
		outTarget = ResolvedImageView();
		return true;
	}

	if (auto* imageView = objects()->resolveSpecfic<ImageAnyView>(fb.viewID, ObjectType::RenderTargetView))
	{
		outTarget = imageView->resolve();
		return true;
	}

	return false;
}

bool FrameExecutor::resolveFrameBufferRenderTargets(const FrameBuffer& fb, FrameBufferTargets& outTargets) const
{
	bool ret = true;
	ret &= resolveFrameBufferRenderTarget(fb.depth, outTargets.images[0]);

	for (uint32_t i=0; i<FrameBuffer::MAX_COLOR_TARGETS; ++i)
		ret &= resolveFrameBufferRenderTarget(fb.color[i], outTargets.images[i+1]);

	return ret;
}

void FrameExecutor::runBeginPass(const OpBeginPass& op)
{
	IFrameExecutor::runBeginPass(op);
				
	if (m_pass.swapchain)
	{
		if (auto* object = objects()->resolveStatic<api::Output>(m_activeSwapchain))
			m_pass.m_valid = object->acquire();
				
		GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		//m_pass.m_glFrameBuffer = 0;

		/*GLenum buffers[1] = { GL_COLOR_ATTACHMENT0 };
		GL_PROTECT(glDrawBuffers(1, buffers));*/
	}
	else
	{
		FrameBufferTargets targets;
		if (resolveFrameBufferRenderTargets(op.frameBuffer, targets))
		{
			if (auto glFrameBuffer = cache()->buildFramebuffer(targets))
			{
				GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, glFrameBuffer));
				//m_pass.m_glFrameBuffer = glFrameBuffer;
				m_pass.m_valid = true;							
			}
		}
	}

	if (m_pass.m_valid)
		resetViewport(op.frameBuffer, op.renderArea);
}

void FrameExecutor::runEndPass(const OpEndPass& op)
{
	if (m_pass.m_valid)
	{
		GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, 0));
		//m_pass.m_glFrameBuffer = 0;
		m_pass.m_valid = false;
	}

	if (m_glIndirectBuffer)
	{
		GL_PROTECT(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0));
		m_glIndirectBuffer = 0;
	}

	m_currentRenderState = StateValues();
	m_currentRenderState.apply(m_passRenderStateMask);
	m_passRenderStateMask = StateMask();
	m_drawRenderStateMask = StateMask();

	IFrameExecutor::runEndPass(op);
}

void FrameExecutor::runBeginBlock(const OpBeginBlock& op)
{
	GL_PROTECT(glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, m_nextDebugMessageID++, -1, op.payload<char>()));
}

void FrameExecutor::runEndBlock(const OpEndBlock& op)
{
	GL_PROTECT(glPopDebugGroup());
}

void FrameExecutor::runCopyRenderTarget(const OpCopyRenderTarget& op)
{
	// get source frame buffer
	bool depthResolve = false;
	GLuint glFrameBuffers[2] = { 0,0 };
	for (int i = 0; i < 2; ++i)
	{
		const auto& view = (i == 0) ? op.sourceView : op.destView;
		if (auto viewObject = objects()->resolveStatic(view, ObjectType::Unknown))
		{
			if (viewObject->objectType() == ObjectType::RenderTargetView)
			{
				auto* rtv = static_cast<ImageAnyView*>(viewObject);

				depthResolve |= GetImageFormatInfo(rtv->setup().format).formatClass == ImageFormatClass::DEPTH;
				const auto targetIndex = depthResolve ? 0 : 1;

				FrameBufferTargets frameBuffer;
				frameBuffer.images[targetIndex] = rtv->resolve();
				frameBuffer.images[targetIndex].firstSlice += op.sourceSlice;
				frameBuffer.images[targetIndex].numSlices = 1;
				frameBuffer.images[targetIndex].numMips = 1;

				glFrameBuffers[i] = cache()->buildFramebuffer(frameBuffer);

			}
			else if (viewObject->objectType() == ObjectType::OutputRenderTargetView)
			{
				glFrameBuffers[i] = 0;
			}
			else
			{
				DEBUG_CHECK(!"Invalid source view");
				return;
			}
		}
	}

    //--

    {
        const auto resolveBit = depthResolve ? GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT;

		auto destY0 = op.flipY ? (op.destRect.max.y-1) : op.destRect.min.y;
		auto destY1 = op.flipY ? (op.destRect.min.x-1) : op.destRect.max.y;

        GL_PROTECT(glBlitNamedFramebuffer(glFrameBuffers[0], glFrameBuffers[1],
            op.sourceRect.min.x, op.sourceRect.min.y, op.sourceRect.max.x, op.sourceRect.max.y,
			op.destRect.min.x, destY0, op.destRect.max.x, destY1,
            resolveBit, GL_NEAREST));
    }
}

void FrameExecutor::runResolve(const OpResolve& op)
{
	// TODO: depth surface

	auto sourceImage = objects()->resolveStatic<Image>(op.source);
	ASSERT_EX(sourceImage, "Resource got removed while command buffer was pending for execution");

	auto targetImage = objects()->resolveStatic<Image>(op.dest);
	ASSERT_EX(targetImage, "Resource got removed while command buffer was pending for execution");

	//--

	const auto depthResolve = GetImageFormatInfo(sourceImage->setup().format).formatClass == ImageFormatClass::DEPTH;
	const auto targetIndex = depthResolve ? 0 : 1;

	FrameBufferTargets sourceFrameBuffer, targetFrameBuffer;
	sourceFrameBuffer.images[targetIndex] = sourceImage->resolve();
	sourceFrameBuffer.images[targetIndex].firstSlice = op.destSlice;
	sourceFrameBuffer.images[targetIndex].firstMip = op.destMip;
	targetFrameBuffer.images[targetIndex] = targetImage->resolve();
	targetFrameBuffer.images[targetIndex].firstSlice = op.destSlice;
	targetFrameBuffer.images[targetIndex].firstMip = op.destMip;

	//--

	const auto glSourceFrameBuffer = cache()->buildFramebuffer(sourceFrameBuffer);
	const auto glTargetFrameBuffer = cache()->buildFramebuffer(targetFrameBuffer);

	//--

	if (glSourceFrameBuffer && glTargetFrameBuffer)
	{
		const auto resolveBit = depthResolve ? GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT;

		const auto sourceWidth = std::max<uint32_t>(1, sourceImage->setup().width >> op.sourceMip);
		const auto sourceHeight = std::max<uint32_t>(1, sourceImage->setup().height >> op.sourceMip);
		const auto targetWidth = std::max<uint32_t>(1, targetImage->setup().width >> op.destMip);
		const auto targetHeight = std::max<uint32_t>(1, targetImage->setup().height >> op.destMip);
		ASSERT_EX(sourceWidth == targetWidth && sourceHeight == targetHeight, TempString("Incomaptible size [{}x{}] != [{}x{}]", sourceWidth, sourceHeight, targetWidth, targetHeight));

		GL_PROTECT(glBlitNamedFramebuffer(glSourceFrameBuffer, glTargetFrameBuffer,
			0, 0, sourceWidth, sourceHeight,
			0, 0, targetWidth, targetHeight,
			resolveBit, GL_NEAREST));
	}
}

void FrameExecutor::runClearPassRenderTarget(const OpClearPassRenderTarget& op)
{
	ASSERT(m_pass.passOp);

	if (m_pass.m_valid)
	{
		if (op.index == 0)
		{
			GL_PROTECT(glClearColor(op.color[0], op.color[1], op.color[2], op.color[3]));
			GL_PROTECT(glClear(GL_COLOR_BUFFER_BIT));
		}
	}
}

void FrameExecutor::runClearPassDepthStencil(const OpClearPassDepthStencil& op)
{
	ASSERT(m_pass.passOp);

	if (m_pass.m_valid)
	{
		uint32_t flags = 0;

		if (op.clearFlags & 1)
		{
			GL_PROTECT(glClearDepthf(op.depthValue));
			GL_PROTECT(glDepthMask(true));
			flags |= GL_DEPTH_BUFFER_BIT;
		}

		if (op.clearFlags & 2)
		{
			GL_PROTECT(glClearStencil(op.stencilValue));
			flags |= GL_STENCIL_BUFFER_BIT;
		}

		GL_PROTECT(glClear(flags));
	}
}


static void ClearImageRect(const ResolvedImageView& view, const ResourceClearRect& rect, const void* clearData)
{
	GLuint glFormat = 0;
	GLuint glType = 0;
	DecomposeTextureFormat(view.glInternalFormat, glFormat, glType);

	switch (view.glViewType)
	{
	case GL_TEXTURE_1D:
		GL_PROTECT(glClearTexSubImage(view.glImage, view.firstMip, rect.image.offsetX, 0, 0, /**/ rect.image.sizeX, 1, 1, /**/ glFormat, glType, clearData));
		break;

	case GL_TEXTURE_1D_ARRAY:
		GL_PROTECT(glClearTexSubImage(view.glImage, view.firstMip, rect.image.offsetX, view.firstSlice, 0, /**/ rect.image.sizeX, view.numSlices, 1, /**/ glFormat, glType, clearData));
		break;

	case GL_TEXTURE_2D:
		GL_PROTECT(glClearTexSubImage(view.glImage, view.firstMip, rect.image.offsetX, rect.image.offsetY, 0, /**/ rect.image.sizeX, rect.image.sizeY, 1, /**/ glFormat, glType, clearData));
		break;

	case GL_TEXTURE_CUBE_MAP:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
	case GL_TEXTURE_2D_ARRAY:
		GL_PROTECT(glClearTexSubImage(view.glImage, view.firstMip, rect.image.offsetX, rect.image.offsetY, view.firstSlice, /**/ rect.image.sizeX, rect.image.sizeY, view.numSlices, /**/ glFormat, glType, clearData));
		break;

	case GL_TEXTURE_3D:
		GL_PROTECT(glClearTexSubImage(view.glImage, view.firstMip, rect.image.offsetX, rect.image.offsetY, rect.image.offsetZ, /**/ rect.image.sizeX, rect.image.sizeY, rect.image.sizeZ, /**/ glFormat, glType, clearData));
		break;
	}
}

static void ClearImageRect2(const ResolvedImageView& view, const Rect& rect, const void* clearData)
{
	GLenum glFormat = GL_RGBA;
	GLenum glType = GL_FLOAT;

	switch (view.glViewType)
	{

	case GL_TEXTURE_2D:
		GL_PROTECT(glClearTexSubImage(view.glImage, view.firstMip, rect.min.x, rect.min.y, view.firstSlice, /**/ rect.width(), rect.height(), 1, /**/ glFormat, glType, clearData));
		break;

	case GL_TEXTURE_CUBE_MAP:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
	case GL_TEXTURE_2D_ARRAY:
		GL_PROTECT(glClearTexSubImage(view.glImage, view.firstMip, rect.min.x, rect.min.y, view.firstSlice, /**/ rect.width(), rect.height(), view.numSlices, /**/ glFormat, glType, clearData));
		break;
	}
}

static void ClearImageRect3(const ResolvedImageView& view, const Rect& rect, bool clearDepth, bool clearStencil, float depthValue, uint32_t stencilValue)
{
	GLuint glFormat = GL_DEPTH_STENCIL;
	GLuint glType = GL_UNSIGNED_INT_24_8;

	const uint32_t depthMaxUint = (1U << 24) - 1;
	const uint32_t clearDepthUint = std::clamp<float>(depthValue * (double)depthMaxUint, 0, depthMaxUint);

	const uint32_t clearValue = (stencilValue & 0xFF) | (clearDepthUint << 8);

	switch (view.glViewType)
	{
	case GL_TEXTURE_2D:
		GL_PROTECT(glClearTexSubImage(view.glImage, view.firstMip, rect.min.x, rect.min.y, view.firstSlice, /**/ rect.width(), rect.height(), 1, /**/ glFormat, glType, &clearValue));
		break;

	case GL_TEXTURE_CUBE_MAP:
	case GL_TEXTURE_CUBE_MAP_ARRAY:
	case GL_TEXTURE_2D_ARRAY:
		GL_PROTECT(glClearTexSubImage(view.glImage, view.firstMip, rect.min.x, rect.min.y, view.firstSlice, /**/ rect.width(), rect.height(), view.numSlices, /**/ glFormat, glType, &clearValue));
		break;
	}
}

void FrameExecutor::runClearRenderTarget(const OpClearRenderTarget& op)
{
	auto* imageViewPtr = objects()->resolveSpecfic<ImageAnyView>(op.view, ObjectType::RenderTargetView);
	ASSERT_EX(imageViewPtr, "Object was removed while command buffer was waiting for submission");

	auto imageResolved = imageViewPtr->resolve();
	DEBUG_CHECK_RETURN_EX(imageResolved, "Internal OOM"); // OOM between recording and execution

	if (op.numRects == 0)
	{
		Rect rect;
		rect.max.x = std::max<uint32_t>(1, imageViewPtr->image()->setup().width >> imageViewPtr->setup().firstMip);
		rect.max.y = std::max<uint32_t>(1, imageViewPtr->image()->setup().height >> imageViewPtr->setup().firstMip);
		ClearImageRect2(imageResolved, rect, op.color);
	}
	else
	{
		const auto* rect = (const Rect*)op.payload();
		for (uint32_t k = 0; k < op.numRects; ++k, ++rect)
			ClearImageRect2(imageResolved, *rect, op.color);
	}
}

void FrameExecutor::runClearDepthStencil(const OpClearDepthStencil& op)
{
	auto* imageViewPtr = objects()->resolveSpecfic<ImageAnyView>(op.view, ObjectType::RenderTargetView);
	ASSERT_EX(imageViewPtr, "Object was removed while command buffer was waiting for submission");

	auto imageResolved = imageViewPtr->resolve();
	DEBUG_CHECK_RETURN_EX(imageResolved, "Internal OOM"); // OOM between recording and execution

	if (op.numRects == 0)
	{
		Rect rect;
		rect.max.x = std::max<uint32_t>(1, imageViewPtr->image()->setup().width >> imageViewPtr->setup().firstMip);
		rect.max.y = std::max<uint32_t>(1, imageViewPtr->image()->setup().height >> imageViewPtr->setup().firstMip);
		ClearImageRect3(imageResolved, rect, op.clearFlags & 1, op.clearFlags & 2, op.depthValue, op.stencilValue);
	}
	else
	{
		const auto* rect = (const Rect*)op.payload();
		for (uint32_t k = 0; k < op.numRects; ++k, ++rect)
			ClearImageRect3(imageResolved, *rect, op.clearFlags & 1, op.clearFlags & 2, op.depthValue, op.stencilValue);
	}
}

static bool IsSwapchain(const FrameBuffer& fb)
{
	if (fb.depth.swapchain)
		return true;
	for (const auto& color : fb.color)
		if (color.swapchain)
			return true;

	return false;
}

void FrameExecutor::runClearFrameBuffer(const OpClearFrameBuffer& op)
{
	bool valid = false;
	if (IsSwapchain(op.frameBuffer))
	{
		if (auto* object = objects()->resolveStatic<api::Output>(m_activeSwapchain))
		{
			if (object->acquire())
			{
				GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, 0));
				valid = true;
			}
		}					
	}
	else
	{
		FrameBufferTargets targets;
		if (resolveFrameBufferRenderTargets(op.frameBuffer, targets))
		{
			if (auto glFrameBuffer = cache()->buildFramebuffer(targets))
			{
				GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, glFrameBuffer));
				valid = true;
			}
		}
	}

	if (valid)
		resetViewport(op.frameBuffer, op.customArea);
}

void FrameExecutor::runClearBuffer(const OpClearBuffer& op)
{
	auto* bufferViewPtr = objects()->resolveStatic<BufferTypedView>(op.view);
	ASSERT_EX(bufferViewPtr, "Object was removed while command buffer was waiting for submission");
	ASSERT_EX(bufferViewPtr->setup().writable, "Non writable view");

	auto bufferResolved = bufferViewPtr->buffer()->resolve();
	DEBUG_CHECK_RETURN_EX(bufferResolved, "Internal OOM"); // OOM between recording and execution

	const auto clearValueSize = GetImageFormatInfo(op.clearFormat).bitsPerPixel / 8;
	const auto glInternalFormat = TranslateImageFormat(op.clearFormat);

	GLuint glFormat = 0;
	GLuint glType = 0;
	DecomposeTextureFormat(glInternalFormat, glFormat, glType);

	const void* clearData = (const uint8_t*)op.payload() + (op.numRects * sizeof(image::ImageRect));

	if (op.numRects == 0)
	{
		if (bufferViewPtr->setup().offset == 0 && bufferViewPtr->setup().size == bufferResolved.size)
		{
			GL_PROTECT(glClearNamedBufferData(bufferResolved.glBuffer, glInternalFormat, glFormat, glType, clearData));
		}
		else
		{
			GL_PROTECT(glClearNamedBufferSubData(bufferResolved.glBuffer, glInternalFormat, bufferViewPtr->setup().offset, bufferViewPtr->setup().size, glFormat, glType, clearData));
		}
	}
	else
	{
		const auto* rect = (const ResourceClearRect*)op.payload();
		for (uint32_t k = 0; k < op.numRects; ++k, ++rect)
		{
			GL_PROTECT(glClearNamedBufferSubData(bufferResolved.glBuffer, glInternalFormat, rect->buffer.offset + bufferViewPtr->setup().offset, rect->buffer.size, glFormat, glType, clearData));
		}
	}
}

void FrameExecutor::runClearStructuredBuffer(const OpClearStructuredBuffer& op)
{
    auto* bufferViewPtr = objects()->resolveStatic<BufferUntypedView>(op.view);
    ASSERT_EX(bufferViewPtr, "Object was removed while command buffer was waiting for submission");
    ASSERT_EX(bufferViewPtr->setup().writable, "Non writable view");

    auto bufferResolved = bufferViewPtr->buffer()->resolve();
    DEBUG_CHECK_RETURN_EX(bufferResolved, "Internal OOM"); // OOM between recording and execution

	uint8_t clearData[32];
	memzero(clearData, sizeof(clearData));

	auto bufferOffset = bufferViewPtr->setup().offset + (op.firstElement * op.stride);
	auto bufferSize = op.numElements * op.stride;

    GL_PROTECT(glClearNamedBufferSubData(bufferResolved.glBuffer, GL_R32UI, bufferOffset, bufferSize, GL_RED, GL_UNSIGNED_INT, clearData));
}

void FrameExecutor::runClearImage(const OpClearImage& op)
{
	auto* imageViewPtr = objects()->resolveSpecfic<ImageAnyView>(op.view, ObjectType::ImageWritableView);
	ASSERT_EX(imageViewPtr, "Object was removed while command buffer was waiting for submission");

	auto imageResolved = imageViewPtr->resolve();
	DEBUG_CHECK_RETURN_EX(imageResolved, "Internal OOM"); // OOM between recording and execution

	const void* clearData = (const uint8_t*)op.payload() + (op.numRects * sizeof(image::ImageRect));
	const auto* clearRects = (const ResourceClearRect*)op.payload();

	if (op.numRects == 0)
	{
		ResourceClearRect rect;
		rect.image.offsetX = 0;
		rect.image.offsetY = 0;
		rect.image.offsetZ = 0;
		rect.image.sizeX = std::max<uint32_t>(1, imageViewPtr->image()->setup().width >> imageViewPtr->setup().firstMip);
		rect.image.sizeY = std::max<uint32_t>(1, imageViewPtr->image()->setup().height >> imageViewPtr->setup().firstMip);
		rect.image.sizeZ = std::max<uint32_t>(1, imageViewPtr->image()->setup().depth >> imageViewPtr->setup().firstMip);
		ClearImageRect(imageResolved, rect, clearData);
	}
	else
	{
		const auto* rect = (const ResourceClearRect*)op.payload();
		for (uint32_t k = 0; k < op.numRects; ++k, ++rect)
			ClearImageRect(imageResolved, *rect, clearData);
	}
}
			
void FrameExecutor::runDownload(const OpDownload& op)
{
    auto destObject = objects()->resolveStatic(op.id, ObjectType::Unknown);
    DEBUG_CHECK_RETURN_EX(destObject, "Destination object lost before command buffer was run (waited more than one frame for submission)");

    auto sinkPtr = DownloadDataSinkPtr(AddRef(op.sink));
	auto objectCache = cache();

	if (auto* destArea = objectCache->allocateDownloadArea(op.dataSize))
	{
		if (destObject->objectType() == ObjectType::Buffer)
		{
			auto* destBuffer = static_cast<Buffer*>(destObject);
			destBuffer->download(destArea, op.range);
		}
        else if (destObject->objectType() == ObjectType::Image)
        {
            auto* destImage = static_cast<Image*>(destObject);
			destImage->download(destArea, op.range);
        }

        thread()->registerCurrentFrameGPUComplectionCallback([destArea, sinkPtr, op, objectCache]() {
			sinkPtr->processRetreivedData(destArea->ptr(), op.dataSize, op.range);
			objectCache->freeDownloadArea(destArea);
            });
	}
	else
	{
		op.sink->processRetreivedData(nullptr, 0, op.range);
	}
}

void FrameExecutor::runDraw(const OpDraw& op)
{
	auto* pso = objects()->resolveStatic<GraphicsPipeline>(op.pipelineObject);
	DEBUG_CHECK_RETURN_EX(pso, "Shaders unloaded while command buffer was pending processing");

	if (prepareDraw(pso, true))
	{
		const auto glTopology = pso->staticRenderState().polygon.topology;
		if (op.numInstances > 1 || op.firstInstance)
		{
			GL_PROTECT(glDrawArraysInstancedBaseInstance(glTopology, op.firstVertex, op.vertexCount, op.numInstances, op.firstInstance));
		}
		else
		{
			GL_PROTECT(glDrawArrays(glTopology, op.firstVertex, op.vertexCount));
		}
	}
}

void FrameExecutor::runDrawIndexed(const OpDrawIndexed& op)
{
	auto* pso = objects()->resolveStatic<GraphicsPipeline>(op.pipelineObject);
	DEBUG_CHECK_RETURN_EX(pso, "Shaders unloaded while command buffer was pending processing");

	if (prepareDraw(pso, true))
	{
		const auto glTopology = pso->staticRenderState().polygon.topology;

		GLenum indexType = 0;
		uint32_t indexSize = 0;
		ASSERT(m_geometry.indexFormat == ImageFormat::R16_UINT || m_geometry.indexFormat == ImageFormat::R32_UINT);
		if (m_geometry.indexFormat == ImageFormat::R16_UINT)
		{
			indexType = GL_UNSIGNED_SHORT;
			indexSize = 2;
		}
		else if (m_geometry.indexFormat == ImageFormat::R32_UINT)
		{
			indexType = GL_UNSIGNED_INT;
			indexSize = 4;
		}

		if (op.numInstances > 1 || op.firstInstance > 0)
		{
			GL_PROTECT(glDrawElementsInstancedBaseVertexBaseInstance(glTopology, op.indexCount, indexType,
				(uint8_t*) nullptr + op.firstIndex * indexSize + m_geometry.finalIndexStreamOffset,
				op.numInstances, op.firstVertex, op.firstInstance));
		}
		else
		{
			GL_PROTECT(glDrawElementsBaseVertex(glTopology, op.indexCount, indexType,
				(uint8_t*) nullptr + op.firstIndex * indexSize + m_geometry.finalIndexStreamOffset,
				op.firstVertex));
		}
	}
}

void FrameExecutor::runDispatch(const OpDispatch& op)
{
	auto* pso = objects()->resolveStatic<ComputePipeline>(op.pipelineObject);
	DEBUG_CHECK_RETURN_EX(pso, "Shaders unloaded while command buffer was pending processing");

	if (prepareDispatch(pso))
	{
		GL_PROTECT(glDispatchCompute(op.counts[0], op.counts[1], op.counts[2]));
	}
}

bool FrameExecutor::prepareIndirectBuffer(ObjectID indirectBufferId)
{
	auto* buffer = objects()->resolveStatic<Buffer>(indirectBufferId);;
	DEBUG_CHECK_RETURN_EX_V(buffer, "Indirect buffer unloaded while command buffer was pending processing", false);
	DEBUG_CHECK_RETURN_EX_V(buffer->setup().allowIndirect, "Buffer not meant for indirect drawing", false);

	auto resolved = buffer->resolve();
	if (m_glIndirectBuffer != resolved.glBuffer)
	{
		m_glIndirectBuffer = resolved.glBuffer;
		GL_PROTECT(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_glIndirectBuffer));
	}

	return true;
}

void FrameExecutor::runDrawIndirect(const OpDrawIndirect& op)
{
	auto* pso = objects()->resolveStatic<GraphicsPipeline>(op.pipelineObject);
	DEBUG_CHECK_RETURN_EX(pso, "Shaders unloaded while command buffer was pending processing");

	if (prepareDraw(pso, false) && prepareIndirectBuffer(op.argumentBuffer))
	{
		const auto glTopology = pso->staticRenderState().polygon.topology;
		GL_PROTECT(glDrawArraysIndirect(glTopology, (void*)op.offset));
	}
}

void FrameExecutor::runDrawIndexedIndirect(const OpDrawIndexedIndirect& op)
{
	auto* pso = objects()->resolveStatic<GraphicsPipeline>(op.pipelineObject);
	DEBUG_CHECK_RETURN_EX(pso, "Shaders unloaded while command buffer was pending processing");

	if (prepareDraw(pso, true) && prepareIndirectBuffer(op.argumentBuffer))
	{
		GLenum indexType = 0;
		ASSERT(m_geometry.indexFormat == ImageFormat::R16_UINT || m_geometry.indexFormat == ImageFormat::R32_UINT);
		if (m_geometry.indexFormat == ImageFormat::R16_UINT)
			indexType = GL_UNSIGNED_SHORT;
		else if (m_geometry.indexFormat == ImageFormat::R32_UINT)
			indexType = GL_UNSIGNED_INT;

		const auto glTopology = pso->staticRenderState().polygon.topology;
		GL_PROTECT(glDrawElementsIndirect(glTopology, indexType, (void*)op.offset));
	}
}

void FrameExecutor::runDispatchIndirect(const OpDispatchIndirect& op)
{

}

static GLuint FlagsForResourceLayout(ResourceLayout layout, bool buffer)
{
	switch (layout)
	{
	case ResourceLayout::ConstantBuffer:
		return GL_UNIFORM_BARRIER_BIT;

	case ResourceLayout::VertexBuffer:
		return GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;

	case ResourceLayout::IndexBuffer:
		return GL_ELEMENT_ARRAY_BARRIER_BIT;

	case ResourceLayout::UAV:
		return GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;

	case ResourceLayout::DepthWrite:
	case ResourceLayout::DepthRead:
	case ResourceLayout::ResolveDest:
	case ResourceLayout::ResolveSource:
	case ResourceLayout::Present:
	case ResourceLayout::RenderTarget:
		return GL_FRAMEBUFFER_BARRIER_BIT;

	case ResourceLayout::ShaderResource:
		if (buffer)
			return GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
		else
			return GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;

	case ResourceLayout::IndirectArgument:
		return GL_COMMAND_BARRIER_BIT;

	case ResourceLayout::CopyDest:
	case ResourceLayout::CopySource:
		if (buffer)
			return GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT;
		else
			return GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT | GL_PIXEL_BUFFER_BARRIER_BIT;

	case ResourceLayout::RayTracingAcceleration:
	case ResourceLayout::ShadingRateSource:
	case ResourceLayout::Common:
		break;
	}

	return GL_ALL_BARRIER_BITS;
}

void FrameExecutor::runResourceLayoutBarrier(const OpResourceLayoutBarrier& op)
{
	GLuint barrierFlags = 0;
	if (const auto obj = objects()->resolveStatic(op.id, ObjectType::Unknown))
	{
		const auto flagBuffer = (obj->objectType() == ObjectType::Buffer);

		GLuint flags = FlagsForResourceLayout(op.sourceLayout, flagBuffer);
		flags |= FlagsForResourceLayout(op.targetLayout, flagBuffer);

		if (flags)
			GL_PROTECT(glMemoryBarrier(flags));
	}

	GL_PROTECT(glMemoryBarrier(GL_ALL_BARRIER_BITS));
}

void FrameExecutor::runUAVBarrier(const OpUAVBarrier& op)
{
	GL_PROTECT(glTextureBarrier());
}

//--

void FrameExecutor::flushDrawRenderStates()
{
	if (m_drawRenderStateMask.word != 0)
	{
		PC_SCOPE_LVL2(FlushDrawRenderStates);

		m_currentRenderState.apply(m_drawRenderStateMask); // set states to api
		m_passRenderStateMask |= m_drawRenderStateMask; // those states will have to be reset when pass ends
	}
}

void FrameExecutor::resetPassRenderStates()
{
	if (m_passRenderStateMask.word != 0)
	{
		PC_SCOPE_LVL2(ResetPassRenderStates);

		m_currentRenderState = StateValues::DEFAULT_STATES();
		m_currentRenderState.apply(m_passRenderStateMask);
	}
}

void FrameExecutor::runSetViewportRect(const OpSetViewportRect& op)
{
	ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");
	ASSERT_EX(op.viewportIndex < m_pass.viewportCount, "Viewport was not enabled in pass");

	const float x = op.rect.left();
	//const float y = (int)m_pass.height - op.rect.top() - op.rect.height();
	const float y = op.rect.top();
	const float w = op.rect.width();
	const float h = op.rect.height();

	glViewportIndexedf(op.viewportIndex, x, y, w, h);
	glDepthRangeIndexed(op.viewportIndex, op.depthMin, op.depthMax);
}

void FrameExecutor::runSetLineWidth(const OpSetLineWidth& op)
{
	ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

	if (m_currentRenderState.polygon.lineWidth != op.width)
	{
		m_currentRenderState.polygon.lineWidth = op.width;
		m_drawRenderStateMask |= State_PolygonLineWidth;
	}
}

/*void FrameExecutor::runSetDepthBias(const OpSetDepthBias& op)
{
	ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

				
}*/

void FrameExecutor::runSetDepthClip(const OpSetDepthClip& op)
{
	ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

	m_currentRenderState.depth.clipMin = op.min;
	m_currentRenderState.depth.clipMax = op.max;
	m_drawRenderStateMask |= State_DepthBiasValues;
}

void FrameExecutor::runSetBlendColor(const OpSetBlendColor& op)
{
	ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

	m_currentRenderState.common.blendColor[0] = op.color[0];
	m_currentRenderState.common.blendColor[1] = op.color[1];
	m_currentRenderState.common.blendColor[2] = op.color[2];
	m_currentRenderState.common.blendColor[3] = op.color[3];
	m_drawRenderStateMask |= State_BlendConstColor;
}

void FrameExecutor::runSetScissorRect(const OpSetScissorRect& op)
{
	ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");
	ASSERT_EX(op.viewportIndex < m_pass.viewportCount, "Viewport was not enabled in pass");

	const auto x = op.rect.left();
	//const auto y = (int)m_pass.height - op.rect.top() - op.rect.height();
	const auto y = op.rect.top();
	const auto w = op.rect.width();
	const auto h = op.rect.height();

	glScissorIndexed(op.viewportIndex, x, y, w, h);
}

void FrameExecutor::runSetStencilReference(const OpSetStencilReference& op)
{
	ASSERT_EX(m_pass.passOp != nullptr, "Not in pass");

	if (m_currentRenderState.stencil.back.ref != op.back)
	{
		m_currentRenderState.stencil.back.ref = op.back;
		m_drawRenderStateMask |= State_StencilBackFuncReferenceMask;
	}

	if (m_currentRenderState.stencil.front.ref != op.front)
	{
		m_currentRenderState.stencil.front.ref = op.front;
		m_drawRenderStateMask |= State_StencilBackFuncReferenceMask;
	}
}

//--

bool FrameExecutor::prepareDraw(GraphicsPipeline* pso, bool usesIndices)
{
	const auto updateMask = pso->staticRenderStateMask() | m_drawRenderStateMask; // we need to make sure we reset also states changes so far
	m_currentRenderState.applyMasked(pso->staticRenderState(), updateMask, m_drawRenderStateMask);

	flushDrawRenderStates();

	if (m_activeVertexLayout != pso->shaders()->vertexLayout())
	{
		m_activeVertexLayout = pso->shaders()->vertexLayout();
		m_dirtyVertexBuffers = true;
	}

	if (m_dirtyVertexBuffers)
		applyVertexData(pso->shaders()->vertexLayout());

	if (usesIndices && m_dirtyIndexBuffer)
		applyIndexData();

	if (m_dirtyDescriptors)
		applyDescriptors(pso->shaders()->descriptorLayout());

	return pso->apply(m_glActiveProgram);
}

bool FrameExecutor::prepareDispatch(ComputePipeline* pso)
{
	if (m_dirtyDescriptors)
		applyDescriptors(pso->shaders()->descriptorLayout());

	return pso->apply(m_glActiveProgram);
}

void FrameExecutor::applyIndexData()
{
	if (m_geometry.indexBinding.id)
	{
		auto buffer = resolveGeometryBufferView(m_geometry.indexBinding.id, m_geometry.indexBinding.offset);
		DEBUG_CHECK_EX(buffer.glBuffer != 0, "Index buffer unloaded while command buffer was being recorded");
		DEBUG_CHECK_EX(glIsBuffer(buffer.glBuffer), "Object is not a buffer");

		GL_PROTECT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.glBuffer));
		m_geometry.finalIndexStreamOffset = buffer.offset;
	}
	else
	{
		GL_PROTECT(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
	}

	m_dirtyIndexBuffer = false;
}

void FrameExecutor::applyVertexData(VertexBindingLayout* layout)
{
	uint32_t numBoundStreams = 0;
	if (layout)
	{
		GL_PROTECT(glBindVertexArray(layout->object())); // TODO: opengl makes it non-const :(

		numBoundStreams = layout->vertexStreams().size();
		for (uint32_t i = 0; i < numBoundStreams; ++i)
		{
			const auto& info = layout->vertexStreams().typedData()[i];
			DEBUG_CHECK_EX(info.index < m_geometry.vertexBindings.size(), TempString("Missing vertex buffer for bind point '{}'", info.name));

			const auto& bufferView = m_geometry.vertexBindings[info.index];
			DEBUG_CHECK_EX(bufferView.id, TempString("Missing vertex buffer for bind point '{}'", info.name));

			auto buffer = resolveGeometryBufferView(bufferView.id, bufferView.offset);
			DEBUG_CHECK_EX(buffer.glBuffer != 0, TempString("Vertex buffer for bind point '{}' unloaded while command buffer was being recorded", info.name));
			DEBUG_CHECK_EX(glIsBuffer(buffer.glBuffer), "Object is not a buffer");

			GL_PROTECT(glBindVertexBuffer(i, buffer.glBuffer, buffer.offset, info.stride));
		}
	}
	else
	{
		GL_PROTECT(glBindVertexArray(0));
		numBoundStreams = 0;
	}

	// unbind buffers from unused slots
	for (uint32_t i = numBoundStreams; i < m_geometry.maxBoundVertexStreams; ++i)
		GL_PROTECT(glBindVertexBuffer(i, 0, 0, 0));
	m_geometry.maxBoundVertexStreams = numBoundStreams;

	m_dirtyVertexBuffers = false;
}

GLuint FrameExecutor::resolveSampler(const gpu::ObjectID& id)
{
	if (auto samplerPtr = objects()->resolveStatic<Sampler>(id))
		return samplerPtr->object();

	return 0;
}

ResolvedImageView FrameExecutor::resolveSampledImageView(const gpu::ObjectID& viewId)
{
	if (auto imagePtr = objects()->resolveSpecfic<ImageAnyView>(viewId, ObjectType::SampledImageView))
		return imagePtr->resolve();

	return ResolvedImageView();
}

ResolvedImageView FrameExecutor::resolveWritableImageView(const gpu::ObjectID& viewId)
{
	if (auto imagePtr = objects()->resolveSpecfic<ImageAnyView>(viewId, ObjectType::ImageWritableView))
		return imagePtr->resolve();

	return ResolvedImageView();
}

ResolvedImageView FrameExecutor::resolveReadOnlyImageView(const gpu::ObjectID& viewId)
{
	if (auto imagePtr = objects()->resolveSpecfic<ImageAnyView>(viewId, ObjectType::ImageReadOnlyView))
		return imagePtr->resolve();

	return ResolvedImageView();
}

ResolvedBufferView FrameExecutor::resolveGeometryBufferView(const gpu::ObjectID& id, uint32_t offset)
{
	if (auto bufferPtr = objects()->resolveStatic<Buffer>(id))
		return bufferPtr->resolve(offset);

	return ResolvedBufferView();
}

ResolvedBufferView FrameExecutor::resolveUntypedBufferView(const gpu::ObjectID& viewId)
{
	if (auto bufferPtr = objects()->resolveStatic<BufferUntypedView>(viewId))
		return bufferPtr->resolve();

	return ResolvedBufferView();
}

ResolvedFormatedView FrameExecutor::resolveTypedBufferView(const gpu::ObjectID& viewId)
{
	if (auto bufferPtr = objects()->resolveStatic<BufferTypedView>(viewId))
		return bufferPtr->resolve();

	return ResolvedFormatedView();
}

void FrameExecutor::applyDescriptors(DescriptorBindingLayout* layout)
{
	layout->prepare();

	for (const auto& elem : layout->elements())
	{
		// do we have parameter ?
		ASSERT_EX(elem.bindPointIndex < m_descriptors.descriptors.size() && m_descriptors.descriptors[elem.bindPointIndex].dataPtr,
			TempString("Missing parameters for '{}', bind point {}, expected layout: {}, {} bounds descriptrs",
				elem.bindPointName, elem.bindPointIndex, elem.bindPointLayout, printBoundDescriptors()));

		// validate expected layout 
		const auto& descriptor = m_descriptors.descriptors[elem.bindPointIndex];
		ASSERT_EX(descriptor.layoutPtr->id() == elem.bindPointLayout, TempString("Incompatible parameter layout for '{}': '{}' != '{}'",
			elem.bindPointName, descriptor.layoutPtr->id(), elem.bindPointLayout));

		// get the source data in descriptor memory
		const auto& entry = descriptor.dataPtr[elem.elementIndex];
		ASSERT_EX(entry.type == elem.objectType, TempString("Incompatible object in param table '{}', member '{}' at index'{}'. Expected {} got {}",
				elem.bindPointName, elem.elementName, elem.elementIndex, elem.objectType, entry.type));

		// validate a valid view is passed
		if (elem.objectType != DeviceObjectViewType::ConstantBuffer)
		{
			ASSERT_EX(entry.id, TempString("Invalid object view in param table '{}', member '{}' at index'{}'. Expected {} got NULL",
				elem.bindPointName, elem.elementName, elem.elementIndex, elem.objectType));
		}

		switch (elem.objectType)
		{
			case DeviceObjectViewType::ConstantBuffer:
			{
				if (entry.inlinedConstants.uploadedDataPtr == nullptr)
				{
					ASSERT_EX(entry.id, "Missing ConstantBufferView in passed descriptor entry bindings");

					const auto view = resolveUntypedBufferView(entry.id);
					ASSERT_EX(view, TempString("Unable to resolve constant buffer view in param table '{}', member '{}' at index'{}'.",
						elem.bindPointName, elem.elementName, elem.elementIndex));
					m_currentResources.bindUniformBuffer(elem.objectSlot, view);
				}
				else
				{
					ASSERT_EX(!entry.id, "Both offset pointer and resource view ID are set");

					const auto* data = entry.inlinedConstants.uploadedDataPtr;

					ResolvedBufferView view;
					view.glBuffer = m_glUniformBuffers[data->bufferIndex];
					view.offset = data->bufferOffset;
					view.size = data->dataSize;

					m_currentResources.bindUniformBuffer(elem.objectSlot, view);
				}
				break;
			}

			case DeviceObjectViewType::BufferWritable:
			case DeviceObjectViewType::Buffer:
			{
				const auto mode = (entry.type == DeviceObjectViewType::Buffer) ? GL_READ_ONLY : GL_READ_WRITE;
				const auto view = resolveTypedBufferView(entry.id);
				ASSERT_EX(view, TempString("Unable to resolve buffer view in param table '{}', member '{}' at index'{}'.",
					elem.bindPointName, elem.elementName, elem.elementIndex));
				m_currentResources.bindImage(elem.objectSlot, view, mode);
				break;
			}

			case DeviceObjectViewType::BufferStructured:
			case DeviceObjectViewType::BufferStructuredWritable:
			{
				const auto mode = (entry.type == DeviceObjectViewType::BufferStructured) ? GL_READ_ONLY : GL_READ_WRITE;
				const auto view = resolveUntypedBufferView(entry.id);
				ASSERT_EX(view, TempString("Unable to resolve structured buffer view in param table '{}', member '{}' at index'{}'.",
					elem.bindPointName, elem.elementName, elem.elementIndex));
				m_currentResources.bindStorageBuffer(elem.objectSlot, view);
				break;
			}

			case DeviceObjectViewType::Image:
			{
				const auto view = resolveReadOnlyImageView(entry.id);
				ASSERT_EX(view, TempString("Unable to resolve image view in param table '{}', member '{}' at index'{}'.",
					elem.bindPointName, elem.elementName, elem.elementIndex));

				ResolvedFormatedView formatedView;
				formatedView.glBufferView = view.glImageView;
				formatedView.glViewFormat = TranslateImageFormat(elem.objectFormat);
				m_currentResources.bindImage(elem.objectSlot, formatedView, GL_READ_ONLY);
				break;
			}

			case DeviceObjectViewType::ImageWritable:
			{
				const auto view = resolveWritableImageView(entry.id);
				ASSERT_EX(view, TempString("Unable to resolve image view in param table '{}', member '{}' at index'{}'.",
					elem.bindPointName, elem.elementName, elem.elementIndex));

				ResolvedFormatedView formatedView;
				formatedView.glBufferView = view.glImageView;
				formatedView.glViewFormat = TranslateImageFormat(elem.objectFormat);
				m_currentResources.bindImage(elem.objectSlot, formatedView, GL_READ_WRITE);
				break;
			}

			case DeviceObjectViewType::SampledImage:
			{
				const auto view = resolveSampledImageView(entry.id);
				ASSERT_EX(view, TempString("Unable to resolve image view in param table '{}', member '{}' at index'{}'.",
					elem.bindPointName, elem.elementName, elem.elementIndex));

				m_currentResources.bindTexture(elem.objectSlot, view);

				if (elem.glStaticSampler)
				{
					m_currentResources.bindSampler(elem.objectSlot, elem.glStaticSampler);
				}
				else if (elem.samplerSlotIndex >= 0)
				{
					const auto& samplerEntry = descriptor.dataPtr[elem.samplerSlotIndex];
					ASSERT_EX(samplerEntry.type == DeviceObjectViewType::Sampler, "Expected sampler");

					GLuint glSampler = resolveSampler(samplerEntry.id);
					ASSERT_EX(view, TempString("Unable to resolve image view in param table '{}', member '{}' at index'{}'.",
						elem.bindPointName, elem.elementName, elem.elementIndex));

					m_currentResources.bindSampler(elem.objectSlot, glSampler);
				}

				break;
			}
		}
	}

	m_dirtyDescriptors = false;
}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
