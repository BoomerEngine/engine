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
#include "glUtils.h"

#include "base/image/include/image.h"
#include "base/image/include/imageView.h"
#include "base/image/include/imageUtils.h"
#include "rendering/device/include/renderingObject.h"
#include "rendering/device/include/renderingBuffer.h"
#include "base/image/include/imageRect.h"

namespace rendering
{
    namespace gl4
    {
        namespace exec
        {

            //---

            void FrameExecutor::runClear(const command::OpClear& op)
            {
				auto* object = m_objectRegistry.resolveStatic(op.view, ObjectType::Invalid);
				ASSERT_EX(object, "Object was removed while command buffer was waiting for submission");
				ASSERT_EX(object->objectType() == ObjectType::BufferTypedView || object->objectType() == ObjectType::ImageView, "Invalid object type for clear");

				if (object->objectType() == ObjectType::BufferTypedView)
				{
					auto bufferViewPtr = static_cast<BufferUntypedView*>(object);

					auto bufferResolved = bufferViewPtr->resolve();
					DEBUG_CHECK_RETURN_EX(bufferResolved, "Internal OOM"); // OOM between recording and execution

					const auto clearValueSize = GetImageFormatInfo(op.clearFormat).bitsPerPixel / 8;
					const auto glInternalFormat = TranslateImageFormat(op.clearFormat);

					GLuint glFormat = 0;
					GLuint glType = 0;
					bool compresed = false;
					DecomposeTextureFormat(glInternalFormat, glFormat, glType, compresed);
					ASSERT_EX(!compresed, "Compressed format used"); // not supported for clear

					base::image::ImageRect fullRect;

					const auto numRects = op.numRects ? op.numRects : 1;
					const auto* rects = (const base::image::ImageRect*) op.payload();
					if (op.numRects)
					{
						for (uint32_t i = 0; i < op.numRects; ++i)
						{
							ASSERT(rects[i].offsetY == 0);
							ASSERT(rects[i].offsetZ == 0);
							ASSERT(rects[i].sizeY == 1);
							ASSERT(rects[i].sizeZ == 1);
							ASSERT(rects[i].offsetX < bufferResolved.size);
							ASSERT(rects[i].offsetX + rects[i].sizeX <= bufferResolved.size);
						}							
					}
					else
					{
						fullRect = base::image::ImageRect(0, 0, 0, bufferResolved.size, 1, 1);
						rects = &fullRect;
					}

					const void* clearData = (const uint8_t*)op.payload() + (op.numRects * sizeof(base::image::ImageRect));

					for (uint32_t k = 0; k < numRects; ++k)
					{
						const auto& r = rects[k];
						const auto clearOffset = bufferResolved.offset + r.offsetX;
						GL_PROTECT(glClearNamedBufferSubData(bufferResolved.glBuffer, glInternalFormat, clearOffset, r.sizeX, glFormat, glType, clearData));
					}
				}
				else if (object->objectType() == ObjectType::ImageView)
				{
					auto imageViewPtr = static_cast<ImageView*>(object);

					auto imageResolved = imageViewPtr->resolveView();
					DEBUG_CHECK_RETURN_EX(imageResolved, "Internal OOM"); // OOM between recording and execution

					const auto topMipWidth = std::max<uint32_t>(imageViewPtr->imageSetup().width >> imageResolved.firstMip, 1);
					const auto topMipHeight = std::max<uint32_t>(imageViewPtr->imageSetup().height >> imageResolved.firstMip, 1);
					const auto topMipDepth = std::max<uint32_t>(imageViewPtr->imageSetup().depth >> imageResolved.firstMip, 1);

					GLuint glFormat = 0;
					GLuint glType = 0;
					bool compresed = false;
					DecomposeTextureFormat(imageResolved.glInternalFormat, glFormat, glType, compresed);
					ASSERT_EX(!compresed, "Compressed format used"); // not supported for clear

					base::image::ImageRect fullRect;

					const auto numRects = op.numRects ? op.numRects : 1;
					const auto* rects = (const base::image::ImageRect*) op.payload();
					if (op.numRects)
					{
						for (uint32_t i = 0; i < op.numRects; ++i)
							ASSERT_EX(rects[i].within(topMipWidth, topMipHeight, topMipDepth), "Overlapping clear regions");
					}
					else
					{
						fullRect = base::image::ImageRect(0, 0, 0, topMipWidth, topMipHeight, topMipDepth);
						rects = &fullRect;
					}

					const void* clearData = (const uint8_t*)op.payload() + (op.numRects * sizeof(base::image::ImageRect));

					const auto clearValueSize = GetImageFormatInfo(op.clearFormat).bitsPerPixel / 8;
					for (uint32_t i = 0; i < imageResolved.numSlices; ++i)
					{
						for (uint32_t k = 0; k < numRects; ++k)
						{
							auto r = rects[k];
							for (uint32_t j = 0; j < imageResolved.numMips; ++j)
							{
								GL_PROTECT(glClearTexSubImage(imageResolved.glImage, imageResolved.firstMip + j, r.offsetX, r.offsetY, r.offsetZ, r.sizeX, r.sizeY, r.sizeZ, glFormat, glType, clearData));
								r = r.downsampled();
							}
						}
					}
				}
            }

			void FrameExecutor::runClearRenderTarget(const command::OpClearRenderTarget& op)
			{
			}

			void FrameExecutor::runClearDepthStencil(const command::OpClearDepthStencil& op)
			{
				
			}

            void FrameExecutor::runUploadConstants(const command::OpUploadConstants& op)
            {
                // nothing, this opcode is placeholder only
            }

			void FrameExecutor::runUploadDescriptor(const command::OpUploadDescriptor& op)
			{
				// nothing, this opcode is placeholder only
			}

			/*
			GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT
								If set, vertex data sourced from buffer objects after the barrier will reflect data written by shaders prior to the barrier.The set of buffer objects affected by this bit is derived from the buffer object bindings used for generic vertex attributes derived from the GL_VERTEX_ATTRIB_ARRAY_BUFFER bindings.
								GL_ELEMENT_ARRAY_BARRIER_BIT
								If set, vertex array indices sourced from buffer objects after the barrier will reflect data written by shaders prior to the barrier.The buffer objects affected by this bit are derived from the GL_ELEMENT_ARRAY_BUFFER binding.
								GL_UNIFORM_BARRIER_BIT
								Shader uniforms sourced from buffer objects after the barrier will reflect data written by shaders prior to the barrier.
								GL_TEXTURE_FETCH_BARRIER_BIT
								Texture fetches from shaders, including fetches from buffer object memory via buffer textures, after the barrier will reflect data written by shaders prior to the barrier.
								GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
								Memory accesses using shader image load, store, and atomic built - in functions issued after the barrier will reflect data written by shaders prior to the barrier.Additionally, image storesand atomics issued after the barrier will not execute until all memory accesses(e.g., loads, stores, texture fetches, vertex fetches) initiated prior to the barrier complete.
								GL_COMMAND_BARRIER_BIT
								Command data sourced from buffer objects by Draw* Indirect commands after the barrier will reflect data written by shaders prior to the barrier.The buffer objects affected by this bit are derived from the GL_DRAW_INDIRECT_BUFFERand GL_DISPATCH_INDIRECT_BUFFER bindings.
								GL_PIXEL_BUFFER_BARRIER_BIT
								Readsand writes of buffer objects via the GL_PIXEL_PACK_BUFFERand GL_PIXEL_UNPACK_BUFFER bindings(via glReadPixels, glTexSubImage2D, etc.) after the barrier will reflect data written by shaders prior to the barrier.Additionally, buffer object writes issued after the barrier will wait on the completion of all shader writes initiated prior to the barrier.
								GL_TEXTURE_UPDATE_BARRIER_BIT
								Writes to a texture via glTex(Sub)Image*, glCopyTex(Sub)Image*, glClearTex* Image, glCompressedTex(Sub)Image*, and reads via glGetTexImage after the barrier will reflect data written by shaders prior to the barrier.Additionally, texture writes from these commands issued after the barrier will not execute until all shader writes initiated prior to the barrier complete.
								GL_BUFFER_UPDATE_BARRIER_BIT
								Reads or writes to buffer objects via any OpenGL API functions that allow modifying their contents reflect data written by shaders prior to the barrier.Additionally, writes via these commands issued after the barrier will wait on the completion of any shader writes to the same memory initiated prior to the barrier.
								GL_QUERY_BUFFER_BARRIER_BIT
								Writes of buffer objects via the GL_QUERY_BUFFER binding after the barrier will reflect data written by shaders prior to the barrier.Additionally, buffer object writes issued after the barrier will wait on the completion of all shader writes initiated prior to the barrier.
								GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT
								Access by the client to persistent mapped regions of buffer objects will reflect data written by shaders prior to the barrier.Note that this may cause additional synchronization operations.
								GL_FRAMEBUFFER_BARRIER_BIT
								Readsand writes via framebuffer object attachments after the barrier will reflect data written by shaders prior to the barrier.Additionally, framebuffer writes issued after the barrier will wait on the completion of all shader writes issued prior to the barrier.
								GL_TRANSFORM_FEEDBACK_BARRIER_BIT
								Writes via transform feedback bindings after the barrier will reflect data written by shaders prior to the barrier.Additionally, transform feedback writes issued after the barrier will wait on the completion of all shader writes issued prior to the barrier.
								GL_ATOMIC_COUNTER_BARRIER_BIT
								Accesses to atomic counters after the barrier will reflect writes prior to the barrier.
								GL_SHADER_STORAGE_BARRIER_BIT
			*/

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

			void FrameExecutor::runResourceLayoutBarrier(const command::OpResourceLayoutBarrier& op)
			{
				GLuint barrierFlags = 0;
				if (const auto obj = m_objectRegistry.resolveStatic(op.id, ObjectType::Invalid))
				{
					const auto flagBuffer = (obj->objectType() == ObjectType::Buffer);

					GLuint flags = FlagsForResourceLayout(op.sourceLayout, flagBuffer);
					flags |= FlagsForResourceLayout(op.targetLayout, flagBuffer);

					if (flags)
						GL_PROTECT(glMemoryBarrier(flags));
				}
			}

			void FrameExecutor::runUAVBarrier(const command::OpUAVBarrier& op)
			{
				GL_PROTECT(glTextureBarrier());
			}

            void FrameExecutor::runUpdate(const command::OpUpdate& op)
            {
				auto stagingBuffer = m_tempStagingBuffer->resolveUntypedView(op.stagingBufferOffset, op.dataBlockSize);

				auto destObject = m_objectRegistry.resolveStatic(op.id, ObjectType::Invalid);
				DEBUG_CHECK_RETURN_EX(destObject, "Destination object lost before command buffer was run (waited more than one frame for submission)");

				if (destObject->objectType() == ObjectType::Image)
				{
					auto* destImageObject = static_cast<Image*>(destObject);
					destImageObject->copyFromBuffer(stagingBuffer, op.range);
				}
				else if (destObject->objectType() == ObjectType::Buffer)
				{
					auto* destBufferObject = static_cast<Buffer*>(destObject);
					destBufferObject->copyFromBuffer(stagingBuffer, op.range);
				}
            }

            void FrameExecutor::runCopy(const command::OpCopy& op)
            {

            }

            //--

            void FrameExecutor::runDownload(const command::OpDownload& op)
            {
                /*// resolve target
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
                target->publish(targetImage);*/
            }

            /*void FrameExecutor::runDownloadBuffer(const command::OpDownloadBuffer& op)
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
            }*/

            //---

            ResolvedImageView FrameExecutor::resolveImageView(const rendering::ObjectID& viewId)
            {
				if (auto imagePtr = m_objectRegistry.resolveStatic<ImageView>(viewId))
					return imagePtr->resolveView();

                return ResolvedImageView();
            }

            ResolvedBufferView FrameExecutor::resolveGeometryBufferView(const rendering::ObjectID& id, uint32_t offset)
            {
				if (auto bufferPtr = m_objectRegistry.resolveStatic<Buffer>(id))
					return bufferPtr->resolveUntypedView(offset);

				return ResolvedBufferView();
            }

			ResolvedBufferView FrameExecutor::resolveUntypedBufferView(const rendering::ObjectID& viewId)
			{
				if (auto bufferPtr = m_objectRegistry.resolveStatic<BufferUntypedView>(viewId))
					return bufferPtr->resolve();

				return ResolvedBufferView();
			}

			ResolvedFormatedView FrameExecutor::resolveTypedBufferView(const rendering::ObjectID& viewId)
			{
				if (auto bufferPtr = m_objectRegistry.resolveStatic<BufferTypedView>(viewId))
					return bufferPtr->resolve();

				return ResolvedFormatedView();
			}

            //---

        } // exec
    } // gl4
} // rendering
