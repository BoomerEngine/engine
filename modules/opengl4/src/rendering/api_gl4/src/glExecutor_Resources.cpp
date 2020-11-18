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
#include "glImage.h"
#include "glSampler.h"
#include "glBuffer.h"
#include "glTempBuffer.h"
#include "glObjectRegistry.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {

            //---

            void FrameExecutor::runClearBuffer(const command::OpClearBuffer& op)
            {
                auto resolvedBuffer = resolveUntypedBufferView(op.view);
                if (0 != resolvedBuffer.glBuffer)
                {
                    uint8_t data = 0;
                    GL_PROTECT(glClearNamedBufferSubData(resolvedBuffer.glBuffer, GL_R8, resolvedBuffer.offset, resolvedBuffer.size, GL_RED, GL_UNSIGNED_BYTE, &data));
                }
            }

            void FrameExecutor::runClearImage(const command::OpClearImage& op)
            {
                /*auto resolvedBuffer = m_objectResolver.resolveUntypedBufferView(op.view, BufferType::Storage);
                if (0 != resolvedBuffer.m_glBuffer)
                {
                    uint8_t data = 0;
                    GL_PROTECT(glClearNamedBufferSubData(resolvedBuffer.m_glBuffer, GL_R8, resolvedBuffer.m_offset, resolvedBuffer.m_size, GL_RED, GL_UNSIGNED_BYTE, &data));
                }*/
            }
            
            void FrameExecutor::runUploadConstants(const command::OpUploadConstants& op)
            {
                // nothing
            }

            void FrameExecutor::runUploadParameters(const command::OpUploadParameters& op)
            {
                // nothing
            }

            void FrameExecutor::runImageLayoutBarrier(const command::OpImageLayoutBarrier& op)
            {
                // no layout transitions in OpenGL
            }

            void FrameExecutor::runGraphicsBarrier(const command::OpGraphicsBarrier& op)
            {
                //  GL_PROTECT(glTextureBarrier());
                GL_PROTECT(glMemoryBarrier(GL_ALL_BARRIER_BITS));
            }

            void FrameExecutor::runUpdateDynamicImage(const command::OpUpdateDynamicImage& op)
            {
                if (auto destImageObject = m_objectRegistry.resolveStatic<Image>(op.view.id()))
                    destImageObject->updateContent(op.view, op.data, op.placementOffset[0], op.placementOffset[1], op.placementOffset[2]);
            }

            void FrameExecutor::runUpdateDynamicBuffer(const command::OpUpdateDynamicBuffer& op)
            {
                // resolve target image
                auto resolvedBuffer = resolveUntypedBufferView(op.view);
                if (resolvedBuffer.glBuffer != 0)
                {
                    // get the staging buffer
                    auto stagingBuffer = m_tempStagingBuffer->resolveUntypedView(op.stagingBufferOffset, op.dataBlockSize);
                    if (stagingBuffer.glBuffer != 0)
                    {
                        GL_PROTECT(glCopyNamedBufferSubData(stagingBuffer.glBuffer, resolvedBuffer.glBuffer, stagingBuffer.offset, resolvedBuffer.offset + op.offset, op.dataBlockSize));
                    }
                }
            }

            void FrameExecutor::runCopyBuffer(const command::OpCopyBuffer& op)
            {

            }

            //--

            void FrameExecutor::runDownloadImage(const command::OpDownloadImage& op)
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
                auto targetImage = base::RefNew<base::image::Image>(pixelFormat, numChannels, texWidth, texHeight, texDepth);
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

            void FrameExecutor::runDownloadBuffer(const command::OpDownloadBuffer& op)
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

            GLuint FrameExecutor::resolveSampler(ObjectID id)
            {
                if (id.isPredefined())
                    return m_device.resolvePredefinedSampler(id.index());

                if (auto samplerPtr = m_objectRegistry.resolveStatic<Sampler>(id))
                    return samplerPtr->deviceObject();

                return m_device.resolvePredefinedSampler(ID_SamplerWrapBiLinear);
            }

            ResolvedImageView FrameExecutor::resolveImageView(const rendering::ImageView& view)
            {
                if (view.id().isPredefined())
                    return m_device.resolvePredefinedImage(view);

                if (auto imagePtr = m_objectRegistry.resolveStatic<Image>(view.id()))
                    return imagePtr->resolveView(view.key());

                return ResolvedImageView();
            }

            ResolvedBufferView FrameExecutor::resolveUntypedBufferView(const rendering::BufferView& view)
            {
                if (auto bufferPtr = m_objectRegistry.resolveStatic<Buffer>(view.id()))
                    return bufferPtr->resolveUntypedView(view);

                return ResolvedBufferView();
            }

            ResolvedFormatedView FrameExecutor::resolveTypedBufferView(const rendering::BufferView& view, ImageFormat format)
            {
                if (auto bufferPtr = m_objectRegistry.resolveStatic<Buffer>(view.id()))
                    return bufferPtr->resolveTypedView(view, format);

                return ResolvedFormatedView();
            }

            //---

        } // exec
    } // gl4
} // rendering
