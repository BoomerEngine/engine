/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver\execution #]
***/

#include "build.h"
#include "glMicropExecutorState.h"
#include "glDriver.h"
#include "glImage.h"
#include "glSampler.h"
#include "glBuffer.h"
#include "glShaderLibraryAdapter.h"
#include "glTransientAllocator.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"

namespace rendering
{
    namespace gl4
    {

        //---

        void ExecutorStateTracker::runClearBuffer(const command::OpClearBuffer& op)
        {
            auto resolvedBuffer = resolveUntypedBufferView(op.view);
            if (0 != resolvedBuffer.glBuffer)
            {
                uint8_t data = 0;
                GL_PROTECT(glClearNamedBufferSubData(resolvedBuffer.glBuffer, GL_R8, resolvedBuffer.offset, resolvedBuffer.size, GL_RED, GL_UNSIGNED_BYTE, &data));
            }
        }

        void ExecutorStateTracker::runClearImage(const command::OpClearImage& op)
        {
            /*auto resolvedBuffer = m_objectResolver.resolveUntypedBufferView(op.view, BufferType::Storage);
            if (0 != resolvedBuffer.m_glBuffer)
            {
                uint8_t data = 0;
                GL_PROTECT(glClearNamedBufferSubData(resolvedBuffer.m_glBuffer, GL_R8, resolvedBuffer.m_offset, resolvedBuffer.m_size, GL_RED, GL_UNSIGNED_BYTE, &data));
            }*/
        }

        void ExecutorStateTracker::runAllocTransientBuffer(const command::OpAllocTransientBuffer& op)
        {
            // nothing
        }

        void ExecutorStateTracker::runUploadConstants(const command::OpUploadConstants& op)
        {
            // nothing
        }

        void ExecutorStateTracker::runUploadParameters(const command::OpUploadParameters& op)
        {
            // nothing
        }

        void ExecutorStateTracker::runImageLayoutBarrier(const command::OpImageLayoutBarrier& op)
        {
            // no layout transitions in OpenGL
        }

        void ExecutorStateTracker::runGraphicsBarrier(const command::OpGraphicsBarrier& op)
        {
            //  GL_PROTECT(glTextureBarrier());
            GL_PROTECT(glMemoryBarrier(GL_ALL_BARRIER_BITS));
        }

        void ExecutorStateTracker::runUpdateDynamicImage(const command::OpUpdateDynamicImage& op)
        {
            // resolve target image
            auto destImageObject = ResolveStaticObject<Image>(op.view.id());
            if (destImageObject != nullptr)
                destImageObject->updateContent(op.view, op.data, op.placementOffset[0], op.placementOffset[1], op.placementOffset[2]);
        }

        void ExecutorStateTracker::runUpdateDynamicBuffer(const command::OpUpdateDynamicBuffer& op)
        {
            // resolve target image
            auto resolvedBuffer = resolveUntypedBufferView(op.view);
            if (resolvedBuffer.glBuffer != 0)
            {
                // get the staging buffer
                auto stagingBuffer = m_frame.resolveStagingArea(op.stagingBufferOffset, op.dataBlockSize);
                if (stagingBuffer.glBuffer != 0)
                {
                    // copy data
                    GL_PROTECT(glCopyNamedBufferSubData(stagingBuffer.glBuffer, resolvedBuffer.glBuffer, stagingBuffer.offset, resolvedBuffer.offset + op.offset, op.dataBlockSize));
                }
            }
        }

        void ExecutorStateTracker::runDownloadImage(const command::OpDownloadImage& op)
        {
            // resolve target
            auto target = op.ptr;
            if (!target)
                return;

            // resolve view
            auto resolvedImageView = resolveImageView(op.image);
            if (resolvedImageView.empty())
            {
                target->publish(nullptr);
                return;
            }

            // TODO: readback for non-2D textures!

            // get the size of the texture view
            int texWidth = 1, texHeight = 1, texDepth = 1;
            GL_PROTECT(glGetTextureLevelParameteriv(resolvedImageView.glImage, 0, GL_TEXTURE_WIDTH, &texWidth));
            GL_PROTECT(glGetTextureLevelParameteriv(resolvedImageView.glImage, 0, GL_TEXTURE_HEIGHT, &texHeight));
            GL_PROTECT(glGetTextureLevelParameteriv(resolvedImageView.glImage, 0, GL_TEXTURE_DEPTH, &texDepth));
            TRACE_INFO("Size of texture to download: [{}x{}x{}]", texWidth, texHeight, texDepth);

            // get the internal format of the texture
            int texInternalFormat = -1;
            GL_PROTECT(glGetTextureLevelParameteriv(resolvedImageView.glImage, 0, GL_TEXTURE_INTERNAL_FORMAT, &texInternalFormat));
            TRACE_INFO("Internal format of texture to download: {}", texInternalFormat);

            // determine pixel format
            auto pixelFormat = base::image::PixelFormat::Uint8_Norm;
            auto numChannels = 4;
            auto downloadFormat = GL_RGBA;
            auto downloadType = GL_UNSIGNED_BYTE;
            if (texInternalFormat == GL_DEPTH24_STENCIL8)
            {
                downloadFormat = GL_DEPTH_COMPONENT;
                downloadType = GL_FLOAT;
                pixelFormat = base::image::PixelFormat::Float32_Raw;
                numChannels = 1;
            }

            // create the image to receive the data
            auto targetImage = base::CreateSharedPtr<base::image::Image>(pixelFormat, numChannels, texWidth, texHeight, texDepth);
            if (!targetImage)
            {
                target->publish(nullptr);
                return;
            }

            // download the data
            {
                auto timeStart = base::NativeTimePoint::Now();
                GL_PROTECT(glGetTextureImage(resolvedImageView.glImage, 0, downloadFormat, downloadType, targetImage->view().dataSize(), targetImage->data()))
                    TRACE_INFO("Downloaded image in {}", TimeInterval(timeStart.timeTillNow().toSeconds()));
            }

            // swap the image before returning (expected)
            base::image::FlipY(targetImage->view());

            // we are done
            target->publish(targetImage);
        }

        void ExecutorStateTracker::runDownloadBuffer(const command::OpDownloadBuffer& op)
        {
            // resolve target
            auto target = op.ptr;
            if (!target)
                return;

            // resolve view
            auto resolvedBufferView = resolveUntypedBufferView(op.buffer);
            if (resolvedBufferView.empty())
            {
                target->publish(nullptr);
                return;
            }

            // calculate offset to download from
            auto downloadOffset = resolvedBufferView.offset + op.downloadOffset;
            auto downloadSize = (op.downloadSize > 0) ? op.downloadSize : resolvedBufferView.size;

            // create output buffer
            auto targetBuffer = base::Buffer::Create(POOL_TEMP, downloadSize);
            memset((void*)targetBuffer.data(), 0xCC, downloadSize);
            if (!targetBuffer)
            {
                target->publish(nullptr);
                return;
            }

            // download the data
            {
                auto timeStart = base::NativeTimePoint::Now();
                GL_PROTECT(glMemoryBarrier(GL_ALL_BARRIER_BITS));
                GL_PROTECT(glGetNamedBufferSubData(resolvedBufferView.glBuffer, downloadOffset, downloadSize, (void*)targetBuffer.data()));
                TRACE_INFO("Downloaded buffer in {} ({})", TimeInterval(timeStart.timeTillNow().toSeconds()), MemSize(downloadSize));
            }

            // we are done
            target->publish(targetBuffer);
        }

        //---

        ResolvedBufferView ExecutorStateTracker::resolveConstantView(const rendering::ConstantsView& view)
        {
            // all constants are handled by the frame transient allocator
            return m_frame.resolveConstants(view);
        }

        GLuint ExecutorStateTracker::resolveSampler(ObjectID id)
        {
            // most of the samplers are predefined
            if (id.isPredefined())
            {
                return m_driver->resolvePredefinedSampler(id.internalIndex());
            }
            // it may also be a static
            else if (id.isStatic())
            {
                auto samplerPtr = ResolveStaticObject<Sampler>(id);
                if (samplerPtr)
                    return samplerPtr->deviceObject();
            }

            return m_driver->resolvePredefinedSampler(ID_SamplerWrapBiLinear);
        }

        ResolvedImageView ExecutorStateTracker::resolveImageView(const rendering::ImageView& view)
        {
            auto id = view.id();

            // handle the predefined images first
            if (id.isPredefined())
            {
                // special case for back buffer
                /*if (id.internalIndex() == ID_BackBufferColor)
                {
                    ASSERT_EX(m_glBackBufferColorTexture!= 0, "Back buffer used as attachment but it was not defined");
                    return ResolvedImageView(m_glBackBufferColorTexture, m_glBackBufferColorTexture, GL_TEXTURE_2D, 0, 1, 0, 1);
                }
                else if (id.internalIndex() == ID_BackBufferDepth)
                {
                    ASSERT_EX(m_glBackBufferDepthTexture != 0, "Back buffer used as attachment but it was not defined");
                    return ResolvedImageView(m_glBackBufferDepthTexture, m_glBackBufferDepthTexture, GL_TEXTURE_2D, 0, 1, 0, 1);
                }*/

                // use device predefined texture
                return m_driver->resolvePredefinedImage(view);
            }
            // it may also be a static
            else if (id.isStatic())
            {
                auto imagePtr = ResolveStaticObject<Image>(id);
                if (imagePtr) // may not resolve if outdated
                    return imagePtr->resolveView(view.key());
            }

            // nope
            return ResolvedImageView();
        }

        ResolvedBufferView ExecutorStateTracker::resolveUntypedBufferView(const rendering::BufferView& view)
        {
            auto id = view.id();

            // no buffer
            if (id.empty())
                return ResolvedBufferView();

            // transient buffer
            if (id.isTransient())
            {
                return m_frame.resolveUntypedBufferView(view);
            }
            // static buffer, they can only be used with compatible types
            else if (id.isStatic())
            {
                auto bufferPtr = ResolveStaticObject<Buffer>(id);
                if (bufferPtr)
                    return bufferPtr->resolveUntypedView(view);
            }

            // no buffer resolved
            return ResolvedBufferView();
        }

        ResolvedFormatedView ExecutorStateTracker::resolveTypedBufferView(const rendering::BufferView& view, ImageFormat format)
        {
            auto id = view.id();

            // no buffer
            if (id.empty())
                return ResolvedFormatedView();

            // transient buffer
            if (id.isTransient())
            {
                return m_frame.resolveTypedBufferView(view, format);
            }
            // static buffer, they can only be used with compatible types
            else if (id.isStatic())
            {
                auto bufferPtr = ResolveStaticObject<Buffer>(id);
                if (bufferPtr)
                    return bufferPtr->resolveTypedView(view, format);
            }

            // no buffer resolved
            return ResolvedFormatedView();
        }

        //---

    } // gl4
} // driver
