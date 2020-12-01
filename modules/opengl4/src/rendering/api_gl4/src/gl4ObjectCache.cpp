/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "gl4ObjectCache.h"
#include "gl4DescriptorLayout.h"
#include "gl4VertexLayout.h"
#include "gl4Thread.h"

namespace rendering
{
	namespace api
	{
		namespace gl4
		{

			//---

			ObjectCache::ObjectCache(Thread* owner)
				: IBaseObjectCache(owner)
			{
				m_framebufferMap.reserve(128);
				m_imageMap.reserve(512);
			}

			ObjectCache::~ObjectCache()
			{
				clearFramebuffers();

				for (auto id : m_samplerMap.values())
					GL_PROTECT(glDeleteSamplers(1, &id));
			}

			IBaseVertexBindingLayout* ObjectCache::createOptimalVertexBindingLayout(const base::Array<ShaderVertexStreamMetadata>& streams)
			{
				return new VertexBindingLayout(owner(), streams);
			}

			IBaseDescriptorBindingLayout* ObjectCache::createOptimalDescriptorBindingLayout(const base::Array<ShaderDescriptorMetadata>& descriptors, const base::Array<ShaderStaticSamplerMetadata>& staticSamplers)
			{
				return new DescriptorBindingLayout(owner(), descriptors, staticSamplers);
			}

			//---

			void ObjectCache::clearFramebuffers()
			{
				for (const auto* fb : m_framebufferMap.values())
				{
					GL_PROTECT(glDeleteFramebuffers(1, &fb->glFrameBuffer));
					base::mem::PoolStats::GetInstance().notifyFree(POOL_API_FRAMEBUFFERS, 1);
				}

				m_imageMap.clearPtr();
				m_framebufferMap.clearPtr();
			}

			void ObjectCache::linkImage(GLuint glImage, CachedFramebuffer* fb)
			{
				if (glImage)
				{
					CachedImageInfo* ret = nullptr;
					if (m_imageMap.find(glImage, ret))
					{
						ASSERT(glImage == ret->glImage);
						ASSERT(!ret->frameBuffers.contains(fb));
					}
					else
					{
						ret = new CachedImageInfo();
						ret->glImage = glImage;
						TRACE_SPAM("Creating frame buffer image tracking for {}", glImage);
					}

					ret->frameBuffers.pushBack(fb);
					TRACE_SPAM("Bound frame buffer image {} to frame buffer {}", glImage, fb->glFrameBuffer);
				}
			}

			GLuint ObjectCache::buildFramebuffer(const FrameBufferTargets& targets)
			{
				// reuse if possible (that's the whole point)
				CachedFramebuffer* ret = nullptr;
				if (m_framebufferMap.find(targets, ret))
					return configureFramebuffer(ret) ? ret->glFrameBuffer : 0;

				// create frame buffer
				GLuint frameBuffer = 0;
				GL_PROTECT(glCreateFramebuffers(1, &frameBuffer));
				if (!frameBuffer)
					return frameBuffer;

				// create cache entry
				ret = new CachedFramebuffer();
				ret->targets = targets;
				ret->glFrameBuffer = frameBuffer;
				m_framebufferMap[targets] = ret;

				TRACE_SPAM("Created frame buffer {}", frameBuffer);
				base::mem::PoolStats::GetInstance().notifyAllocation(POOL_API_FRAMEBUFFERS, 1);

				// bind used images to the FB
				for (auto target : targets.images)
					linkImage(target.glImage, ret);

				// configure the newly created frame buffer
				return configureFramebuffer(ret) ? ret->glFrameBuffer : 0;
			}

			const GLenum FrameBufferTargets::glAttachmentNames[MAX_FRAMEBUFFER_TARGETS] = {
				GL_DEPTH_STENCIL_ATTACHMENT,
				GL_COLOR_ATTACHMENT0,
				GL_COLOR_ATTACHMENT1,
				GL_COLOR_ATTACHMENT2,
				GL_COLOR_ATTACHMENT3,
				GL_COLOR_ATTACHMENT4,
				GL_COLOR_ATTACHMENT5,
				GL_COLOR_ATTACHMENT6,
				GL_COLOR_ATTACHMENT7,
			};

			bool ObjectCache::configureFramebuffer(CachedFramebuffer* ret)
			{
				if (ret->configured)
					return true;

				for (uint32_t i=0; i<FrameBufferTargets::MAX_FRAMEBUFFER_TARGETS; ++i)
				{
					const auto& image = ret->targets.images[i];
					if (!image.glImage)
						break;

					const auto glAttachmentType = FrameBufferTargets::glAttachmentNames[i];
					if (image.glViewType == GL_RENDERBUFFER)
					{
						GL_PROTECT(glNamedFramebufferRenderbuffer(ret->glFrameBuffer, glAttachmentType, GL_RENDERBUFFER, image.glImage));
					}
					else if (image.glViewType == GL_TEXTURE_2D)
					{
						GL_PROTECT(glNamedFramebufferTexture(ret->glFrameBuffer, glAttachmentType, image.glImage, image.firstMip));
					}
					else if (image.glViewType == GL_TEXTURE_2D_ARRAY)
					{
						GL_PROTECT(glNamedFramebufferTextureLayer(ret->glFrameBuffer, glAttachmentType, image.glImage, image.firstMip, image.firstSlice));
					}
					else if (image.glViewType == GL_TEXTURE_2D_MULTISAMPLE)
					{
						GL_PROTECT(glNamedFramebufferTexture(ret->glFrameBuffer, glAttachmentType, image.glImage, image.firstMip));
					}
				}

				// check the frame buffer
				GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				DEBUG_CHECK_RETURN_EX_V(status == GL_FRAMEBUFFER_COMPLETE, "Framebuffer setup is not complete", false);

				// save as configured
				ret->configured = true;
				return true;
			}

			void ObjectCache::notifyImageDeleted(GLuint glImage)
			{
				// if we were tracking that image then delete all framebuffers it's in
				CachedImageInfo* ret = nullptr;
				if (m_imageMap.find(glImage, ret))
				{
					ASSERT(ret->glImage == glImage);

					for (auto* fb : ret->frameBuffers)
					{
						m_framebufferMap.remove(fb->targets);
						delete fb;
					}

					TRACE_SPAM("Removed tracking for render target {} ({} framebuffers released)", glImage, ret->frameBuffers.size());

					m_imageMap.remove(glImage);
					delete ret;
				}
			}

			//--

			static GLenum TranslateFilter(const FilterMode mode)
			{
				switch (mode)
				{
					case FilterMode::Nearest:
					return GL_NEAREST;

					case FilterMode::Linear:
						return GL_LINEAR;
				}

				FATAL_ERROR("Invalid filter mode");
				return GL_LINEAR;
			}

			static GLenum TranslateFilter(const FilterMode mode, const MipmapFilterMode mipMode)
			{
				switch (mode)
				{
				case FilterMode::Nearest:
				{
					switch (mipMode)
					{
						case MipmapFilterMode::None: return GL_NEAREST;
						case MipmapFilterMode::Nearest: return GL_NEAREST_MIPMAP_NEAREST;
						case MipmapFilterMode::Linear: return GL_NEAREST_MIPMAP_LINEAR;
					}

					FATAL_ERROR("Invalid mipmap mode");
					return GL_NEAREST;
				}

				case FilterMode::Linear:
				{
					switch (mipMode)
					{
						case MipmapFilterMode::None: return GL_LINEAR;
						case MipmapFilterMode::Nearest: return GL_LINEAR_MIPMAP_NEAREST;
						case MipmapFilterMode::Linear: return GL_LINEAR_MIPMAP_LINEAR;
					}

					FATAL_ERROR("Invalid mipmap mode");
					return GL_LINEAR;
				}
				}

				FATAL_ERROR("Invalid filter mode");
				return GL_LINEAR;
			}

			static GLenum TranslateAddressMode(const AddressMode mode)
			{
				switch (mode)
				{
					case AddressMode::Wrap: return GL_REPEAT;
					case AddressMode::Mirror: return GL_MIRRORED_REPEAT;
					case AddressMode::Clamp: return GL_CLAMP_TO_EDGE;
					case AddressMode::ClampToBorder: return GL_CLAMP_TO_BORDER;
					case AddressMode::MirrorClampToEdge: return GL_MIRROR_CLAMP_TO_EDGE;
				}

				FATAL_ERROR("Invalid mipmap filter mode");
				return GL_REPEAT;
			}

			static GLenum TranslateCompareOp(const CompareOp mode)
			{
				switch (mode)
				{
					case CompareOp::Never: return GL_NEVER;
					case CompareOp::Less: return GL_LESS;
					case CompareOp::LessEqual: return GL_LEQUAL;
					case CompareOp::Greater: return GL_GREATER;
					case CompareOp::NotEqual: return GL_NOTEQUAL;
					case CompareOp::GreaterEqual: return GL_GEQUAL;
					case CompareOp::Always: return GL_ALWAYS;
				}

				FATAL_ERROR("Invalid depth comparision op");
				return GL_ALWAYS;
			}

			static base::Vector4 TranslateBorderColor(const BorderColor mode)
			{
				switch (mode)
				{
					case BorderColor::FloatTransparentBlack: return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
					case BorderColor::FloatOpaqueBlack: return base::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
					case BorderColor::FloatOpaqueWhite: return base::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
					case BorderColor::IntTransparentBlack: return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
					case BorderColor::IntOpaqueBlack: return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
					case BorderColor::IntOpaqueWhite: return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
				}

				FATAL_ERROR("Invalid mipmap filter mode");
				return base::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}

			GLuint ObjectCache::createSampler(const SamplerState& state)
			{
				GLuint glSampler = 0;
				if (m_samplerMap.find(state, glSampler))
					return glSampler;

				auto borderColor = TranslateBorderColor(state.borderColor);

				// create sampler object
				GL_PROTECT(glGenSamplers(1, &glSampler));
				GL_PROTECT(glSamplerParameteri(glSampler, GL_TEXTURE_MIN_FILTER, TranslateFilter(state.minFilter, state.mipmapMode)));
				GL_PROTECT(glSamplerParameteri(glSampler, GL_TEXTURE_MAG_FILTER, TranslateFilter(state.magFilter)));
				GL_PROTECT(glSamplerParameteri(glSampler, GL_TEXTURE_WRAP_S, TranslateAddressMode(state.addresModeU)));
				GL_PROTECT(glSamplerParameteri(glSampler, GL_TEXTURE_WRAP_T, TranslateAddressMode(state.addresModeV)));
				GL_PROTECT(glSamplerParameteri(glSampler, GL_TEXTURE_WRAP_R, TranslateAddressMode(state.addresModeW)));
				GL_PROTECT(glSamplerParameterfv(glSampler, GL_TEXTURE_BORDER_COLOR, (GLfloat*)&borderColor));
				GL_PROTECT(glSamplerParameterf(glSampler, GL_TEXTURE_MIN_LOD, state.minLod));
				GL_PROTECT(glSamplerParameterf(glSampler, GL_TEXTURE_MAX_LOD, state.maxLod));
				GL_PROTECT(glSamplerParameterf(glSampler, GL_TEXTURE_LOD_BIAS, state.mipLodBias));
				GL_PROTECT(glSamplerParameteri(glSampler, GL_TEXTURE_COMPARE_MODE, state.compareEnabled ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE));
				GL_PROTECT(glSamplerParameteri(glSampler, GL_TEXTURE_COMPARE_FUNC, TranslateCompareOp(state.compareOp)));

				m_samplerMap[state] = glSampler;
				return glSampler;
			}

			//--

		} // gl4
	} // api
} // rendering
