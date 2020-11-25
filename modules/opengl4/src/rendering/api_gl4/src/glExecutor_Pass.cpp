/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\frame\execution #]
***/

#include "build.h"
#include "glExecutor.h"
#include "glDevice.h"
#include "glDeviceThread.h"
#include "glUtils.h"
#include "glImage.h"
#include "glObjectRegistry.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {
        
            //---

            void FrameExecutor::runClearPassRenderTarget(const command::OpClearPassRenderTarget& op)
            {
                DEBUG_CHECK_EX(m_pass.passOp != nullptr, "No active pass to run this command in");
                if (m_pass.passOp)
                {
                    if (op.index == 0)
                    {
                        GL_PROTECT(glClearColor(op.color[0], op.color[1], op.color[2], op.color[3]));
                        GL_PROTECT(glClear(GL_COLOR_BUFFER_BIT));
                    }
                }
            }

            void FrameExecutor::runClearPassDepthStencil(const command::OpClearPassDepthStencil& op)
            {
                DEBUG_CHECK_EX(m_pass.passOp != nullptr, "No active pass to run this command in");
                if (m_pass.passOp)
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

            static bool IsSwapChain(const FrameBuffer& fb)
            {
                return fb.color[0].swapchain || fb.depth.swapchain;
            }

            // clears
            struct ColorClearInfo
            {
                bool m_clear;

                union
                {
                    float m_float[4];
                    uint32_t m_uint[4];
                };

                inline ColorClearInfo()
                    : m_clear(false)
                {}
            };

            void FrameExecutor::runBeginPass(const command::OpBeginPass& op)
            {
				DEBUG_CHECK_EX(op.frameBuffer.validate(), "Begin pass with invalid frame buffer should not be recorded");
				DEBUG_CHECK_EX(op.numViewports >= 1 && op.numViewports <= 16, "Invalid viewport count");

                // determine rendering area size as we go
                m_pass.width = 0;
                m_pass.height = 0;
                m_pass.samples = 0;

                // reset state
				DEBUG_CHECK_EX(!m_pass.passOp, "Theres already an active pass");
                m_pass.passOp = &op;

                // collect clear setup
                uint32_t numColorTargets = 0;
                ColorClearInfo clearColor[16];
                ColorClearInfo clearDepth;

                // special output frame buffer ?
                if (IsSwapChain(op.frameBuffer))
                {
					ASSERT_EX(m_activeSwapchain, "Invalid swapchain");

                    m_thread.bindOutput(m_activeSwapchain);
                    GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, 0));

                    // color clear
                    {
                        const auto& att = op.frameBuffer.color[0];
                        if (att.viewID)
                        {
                            if (att.loadOp == LoadOp::Clear)
                            {
                                auto& clearInfo = clearColor[0];
                                clearInfo.m_clear = true;
                                clearInfo.m_float[0] = att.clearColorValues[0];
                                clearInfo.m_float[1] = att.clearColorValues[1];
                                clearInfo.m_float[2] = att.clearColorValues[2];
                                clearInfo.m_float[3] = att.clearColorValues[3];
                            }
                            numColorTargets += 1;
                        }
                    }

                    // depth clear
                    {
                        const auto& att = op.frameBuffer.depth;
                        if (att.viewID)
                        {
                            if (att.loadOp == LoadOp::Clear)
                            {
                                auto& clearInfo = clearDepth;
                                clearInfo.m_clear = true;
                                clearInfo.m_float[0] = att.clearDepthValue;
                                clearInfo.m_uint[1] = att.clearStencilValue;
                            }
                        }
                    }

                    // swapchain frame buffer has index 0
                    m_pass.width = op.frameBuffer.color[0].width;
                    m_pass.height = op.frameBuffer.color[0].height;
                    m_pass.samples = op.frameBuffer.color[0].samples;
                    m_pass.fbo = 0;
                }
                else
                {
                    // TODO: add cache

                    // create frame buffer
                    GLuint frameBuffer;
                    GL_PROTECT(glGenFramebuffers(1, &frameBuffer));
                    GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer));
                    base::mem::PoolStats::GetInstance().notifyAllocation(POOL_API_FRAMEBUFFERS, 1);

                    // attach the color buffers to the frame buffer
                    for (uint32_t i = 0; i < FrameBuffer::MAX_COLOR_TARGETS; ++i)
                    {
                        const auto& att = op.frameBuffer.color[i];
                        if (att.empty())
                            break;

                        // resolve the attached view
                        auto imageView = resolveImageView(att.viewID);
                        if (0 == imageView.glImageView)
                        {
                            TRACE_ERROR(base::TempString("Invalid GL image for a color render target {}: {}", i, att));
                            continue;
                        }

                        // bind the target
                        if (imageView.glImageViewType == GL_TEXTURE_2D || imageView.glImageViewType == GL_TEXTURE_2D_MULTISAMPLE)
                        {
                            GL_PROTECT(glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, imageView.glImageView, 0));
                        }
                        else
                        {
                            TRACE_ERROR(base::TempString("Invalid frame buffer color attachment type {}", imageView.glImageViewType));
                        }

                        // set size of render area
                        if (m_pass.width == 0 && m_pass.height == 0 && m_pass.samples == 0)
                        {
                            m_pass.width = att.width;
                            m_pass.height = att.height;
                            m_pass.samples = att.samples;
                        }
                        else
                        {
                            DEBUG_CHECK_EX(att.width == m_pass.width && att.height == m_pass.height, "Render targets have different sizes");
                            DEBUG_CHECK_EX(att.samples == m_pass.samples, "Render targets have different sample count");
                        }

                        // clear this target ?
                        if (att.loadOp == LoadOp::Clear)
                        {
                            // determine the clear color - it may be specified in the pipeline or passed dynamically
                            auto& clearInfo = clearColor[i];
                            clearInfo.m_clear = true;
                            clearInfo.m_float[0] = att.clearColorValues[0];
                            clearInfo.m_float[1] = att.clearColorValues[1];
                            clearInfo.m_float[2] = att.clearColorValues[2];
                            clearInfo.m_float[3] = att.clearColorValues[3];
                        }

                        // count set color targets
                        numColorTargets += 1;
                    }

                    // attach the depth buffer
                    if (!op.frameBuffer.depth.empty())
                    {
                        const auto& att = op.frameBuffer.depth;

                        // set size of render area
                        if (m_pass.width == 0 && m_pass.height == 0 && m_pass.samples == 0)
                        {
                            m_pass.width = att.width;
                            m_pass.height = att.height;
                            m_pass.samples = att.samples;
                        }
                        else
                        {
                            DEBUG_CHECK_EX(att.width == m_pass.width && att.height == m_pass.height,
                                base::TempString("Render targets have different sizes: [{}x{}] != [{}x{}]", att.width, att.height, m_pass.width, m_pass.height));
                            DEBUG_CHECK_EX(att.samples == m_pass.samples, "Render targets have different sample count");
                        }

                        // resolve the attached view
                        auto imageView = resolveImageView(att.viewID);

                        // invalid texture
                        if (0 == imageView.glImageView)
                        {
                            auto imageViewEx = resolveImageView(att.viewID);
                            TRACE_ERROR(base::TempString("Invalid GL image for a depth stencil target view in pass {}", imageViewEx.glImage));
                        }

                        // bind the target
                        if (imageView.glImageViewType == GL_RENDERBUFFER)
                        {
                            GL_PROTECT(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, imageView.glImageView));
                        }
                        else if (imageView.glImageViewType == GL_TEXTURE_2D)
                        {
                            GL_PROTECT(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, imageView.glImageView, 0));
                        }
                        else if (imageView.glImageViewType == GL_TEXTURE_2D_MULTISAMPLE)
                        {
                            GL_PROTECT(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, imageView.glImageView, 0));
                        }
                        else
                        {
                            TRACE_ERROR(base::TempString("Invalid frame buffer depth attachment type {}", imageView.glImageViewType));
                        }

                        // clear this target ?
                        if (att.loadOp == LoadOp::Clear)
                        {
                            auto& clearInfo = clearDepth;
                            clearInfo.m_clear = true;
                            clearInfo.m_float[0] = att.clearDepthValue;
                            clearInfo.m_uint[1] = att.clearStencilValue;
                        }
                    }

                    m_pass.fbo = frameBuffer;
                }

                // check the frame buffer
                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                ASSERT_EX(status == GL_FRAMEBUFFER_COMPLETE, "Framebuffer setup is not complete");

                // setup initial viewports and clips
                bool requiresEnabledScissor = false;
                m_pass.viewportCount = op.numViewports;
                if (op.hasInitialViewportSetup)
                {
                    const auto* viewportStates = (const FrameBufferViewportState*)op.payload();
                    for (uint8_t i = 0; i < op.numViewports; ++i)
                        applyViewportScissorState(i, viewportStates[i], requiresEnabledScissor);
                }
                else
                {
                    FrameBufferViewportState defaultState;
                    defaultState.viewportRect = base::Rect(0, 0, m_pass.width, m_pass.height);
                    defaultState.scissorRect = base::Rect(0, 0, m_pass.width, m_pass.height);
                    defaultState.minDepthRange = 0.0f;
                    defaultState.maxDepthRange = 1.0f;

                    for (uint8_t i = 0; i < op.numViewports; ++i)
                        applyViewportScissorState(i, defaultState, requiresEnabledScissor);
                }

                // enable scissor
                if (requiresEnabledScissor != m_render.scissorEnabled)
                {
                    m_render.scissorEnabled = false;// requiresEnabledScissor;
                    m_dirtyRenderStates.flags |= RenderStateDirtyBit::ScissorEnabled;
                    m_passChangedRenderStates.flags |= RenderStateDirtyBit::ScissorEnabled;
                }

                // change render target count and with that new count check if we still need to blend
                m_pass.colorCount = numColorTargets;
                updateBlendingEnable();

                // apply render states now to set viewports and scissor rects
                applyDirtyRenderStates();

                // clear color
                for (uint32_t i=0; i<numColorTargets; ++i)
                {
                    auto& clearInfo = clearColor[i];
                    if (clearInfo.m_clear)
                    {
                        GL_PROTECT(glClearBufferfv(GL_COLOR, i, &clearInfo.m_float[0]));
                    }
                }

                // clear depth
                if (clearDepth.m_clear)
                {
                    GL_PROTECT(glClearBufferfi(GL_DEPTH_STENCIL, 0, clearDepth.m_float[0], clearDepth.m_uint[1]));
                }

                // re-enable blending
                updateBlendingEnable();
            }

            void FrameExecutor::applyViewportScissorState(uint8_t index, const FrameBufferViewportState& vs, bool& usesStencil)
            {
                {
                    float x, y, w, h;

                    if (vs.viewportRect.empty())
                    {
                        x = 0.0f; 
                        y = 0.0f;
                        w = m_pass.width;
                        h = m_pass.height;
                    }
                    else
                    {
                        x = vs.viewportRect.left();
                        y = vs.viewportRect.top();
                        w = vs.viewportRect.width();
                        h = vs.viewportRect.height();
                    }

                    auto& targetRect = m_render.viewport[index].rect;
                    if (targetRect[0] != x || targetRect[1] != y || targetRect[2] != w || targetRect[3] != h)
                    {
                        targetRect[0] = x;
                        targetRect[1] = y;
                        targetRect[2] = w;
                        targetRect[3] = h;

                        m_dirtyRenderStates.viewportDirtyPerVP |= (1UL << index);
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::ViewportRects;
                        m_passChangedRenderStates.viewportDirtyPerVP |= (1UL << index);
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::ViewportRects;
                    }
                }

                {
                    int x, y, w, h;

                    if (vs.scissorRect.empty())
                    {
                        x = 0;
                        y = 0;
                        w = m_pass.width;
                        h = m_pass.height;
                    }
                    else
                    {
                        x = vs.scissorRect.left();
                        y = vs.scissorRect.top();
                        w = vs.scissorRect.width();
                        h = vs.scissorRect.height();

                        if (x != 0 || y != 0 || w != m_pass.width || h != m_pass.height)
                            usesStencil = true;
                    }

                    auto& targetRect = m_render.viewport[index].scissor;
                    if (targetRect[0] != x || targetRect[1] != y || targetRect[2] != w || targetRect[3] != h)
                    {
                        targetRect[0] = x;
                        targetRect[1] = y;
                        targetRect[2] = w;
                        targetRect[3] = h;

                        m_dirtyRenderStates.scissorDirtyPerVP |= (1UL << index);
                        m_dirtyRenderStates.flags |= RenderStateDirtyBit::ScissorRects;
                        m_passChangedRenderStates.scissorDirtyPerVP |= (1UL << index);
                        m_passChangedRenderStates.flags |= RenderStateDirtyBit::ScissorRects;
                    }
                }
            }

            void FrameExecutor::runEndPass(const command::OpEndPass& op)
            {
                DEBUG_CHECK_EX(m_pass.passOp, "No pass active");

                // we are no longer rendering
                m_pass.width = 0;
                m_pass.height = 0;

                // reset state
                m_pass.passOp = nullptr;

                // unbind frame buffer
                if (m_pass.fbo != 0)
                {
                    GL_PROTECT(glBindFramebuffer(GL_FRAMEBUFFER, 0));
                    GL_PROTECT(glDeleteFramebuffers(1, &m_pass.fbo));
                    m_pass.fbo = 0;

                    base::mem::PoolStats::GetInstance().notifyFree(POOL_API_FRAMEBUFFERS, 1);
                }

                // restore rendering state to defaults after pass ends
                // NOTE: we don't apply them but we set all restored states as dirty so they will be applied next time we draw()
                m_render = GDefaultState;
                m_dirtyRenderStates |= m_passChangedRenderStates;
                m_passChangedRenderStates = RenderDirtyStateTrack();
            }

			void FrameExecutor::runResolve(const command::OpResolve& op)
			{
				// TODO: depth surface

				auto sourceImage = m_objectRegistry.resolveStatic<Image>(op.source);
				ASSERT_EX(sourceImage, "Resource got removed while command buffer was pending for execution");

				auto targetImage = m_objectRegistry.resolveStatic<Image>(op.dest);
				ASSERT_EX(targetImage, "Resource got removed while command buffer was pending for execution");

				auto sourceView = sourceImage->resolveMainView();
				ASSERT_EX(sourceView, "Unable to resource source view");

				auto targetView = targetImage->resolveMainView();
				ASSERT_EX(targetView, "Unable to resource target view");

				// determine type
				const auto depthResolve = GetImageFormatInfo(sourceImage->creationSetup().format).formatClass == ImageFormatClass::DEPTH;
				const auto attachmentType = depthResolve ? GL_DEPTH_ATTACHMENT : GL_COLOR_ATTACHMENT0;
				const auto resolveBit = depthResolve ? GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT;

				// prepare target frame buffer
				GLuint targetFrameBuffer;
				{
					GL_PROTECT(glCreateFramebuffers(1, &targetFrameBuffer));

					// bind target
					ASSERT_EX(targetImage->creationSetup().view == ImageViewType::View2D || targetImage->creationSetup().view == ImageViewType::View2DArray, "Unexpected target texture type");
					ASSERT_EX(op.destMip < targetImage->creationSetup().numMips, "Invalid target mip index");
					ASSERT_EX(op.destSlice < targetImage->creationSetup().numSlices, "Invalid target slice index");
					if (targetImage->creationSetup().view == ImageViewType::View2D)
					{
						GL_PROTECT(glNamedFramebufferTexture(targetFrameBuffer, attachmentType, targetView.glImage, op.destMip));
					}
					else
					{
						GL_PROTECT(glNamedFramebufferTextureLayer(targetFrameBuffer, attachmentType, targetView.glImage, op.destMip, op.destSlice));
					}

					auto targetStatus = glCheckNamedFramebufferStatus(targetFrameBuffer, GL_DRAW_FRAMEBUFFER);
					ASSERT_EX(targetStatus == GL_FRAMEBUFFER_COMPLETE, "Invalid framebuffer status");
				}

                // bind source
				GLuint sourceFrameBuffer;
				{
					GL_PROTECT(glCreateFramebuffers(1, &sourceFrameBuffer));
					ASSERT_EX(sourceImage->creationSetup().view == ImageViewType::View2D || sourceImage->creationSetup().view == ImageViewType::View2DArray, "Unexpected texture type");
					ASSERT_EX(op.sourceMip < sourceImage->creationSetup().numMips, "Invalid source mip index");
					ASSERT_EX(op.sourceSlice < sourceImage->creationSetup().numSlices, "Invalid source slice index");
					if (sourceImage->creationSetup().view == ImageViewType::View2D)
					{
						GL_PROTECT(glNamedFramebufferTexture(sourceFrameBuffer, attachmentType, sourceView.glImage, op.sourceMip));
					}
					else
					{
						GL_PROTECT(glNamedFramebufferTextureLayer(sourceFrameBuffer, attachmentType, sourceView.glImage, op.sourceMip, op.sourceSlice));
					}
					
					auto sourceStatus = glCheckNamedFramebufferStatus(sourceFrameBuffer, GL_DRAW_FRAMEBUFFER);
					ASSERT_EX(sourceStatus == GL_FRAMEBUFFER_COMPLETE, "Invalid framebuffer status");
				}

				// resolve
				{
					const auto sourceWidth = std::max<uint32_t>(1, sourceImage->creationSetup().width >> op.sourceMip);
					const auto sourceHeight = std::max<uint32_t>(1, sourceImage->creationSetup().height >> op.sourceMip);
					const auto targetWidth = std::max<uint32_t>(1, targetImage->creationSetup().width >> op.destMip);
					const auto targetHeight = std::max<uint32_t>(1, targetImage->creationSetup().height >> op.destMip);
					ASSERT_EX(sourceWidth == targetWidth && sourceHeight == targetHeight, base::TempString("Incomaptible size [{}x{}] != [{}x{}]", sourceWidth, sourceHeight, targetWidth, targetHeight));

                    GL_PROTECT(glBlitNamedFramebuffer(sourceFrameBuffer, targetFrameBuffer,
                        0, 0, sourceWidth, sourceHeight,
                        0, 0, targetWidth, targetHeight,
						resolveBit, GL_NEAREST));
                }

                // destroy
                GL_PROTECT(glDeleteFramebuffers(1, &sourceFrameBuffer));
                GL_PROTECT(glDeleteFramebuffers(1, &targetFrameBuffer));
            }

            //---

        } // exex
    } // gl4
} // rendering
