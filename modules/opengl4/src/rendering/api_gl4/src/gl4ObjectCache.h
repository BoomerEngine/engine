/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiObjectCache.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{
			//---

			struct FrameBufferTargets
			{
				static const uint32_t MAX_FRAMEBUFFER_TARGETS = 9;

				ResolvedImageView images[MAX_FRAMEBUFFER_TARGETS]; // depth is 0, color is 1-9

				static const GLenum glAttachmentNames[MAX_FRAMEBUFFER_TARGETS];

				INLINE FrameBufferTargets()
				{
					memzero(&images, sizeof(images));
				}

				INLINE FrameBufferTargets(const FrameBufferTargets& other)
				{
					memcpy(&images, &other.images, sizeof(images));
				}

				INLINE FrameBufferTargets& operator=(const FrameBufferTargets& other)
				{
					memcpy(&images, &other.images, sizeof(images));
					return *this;
				}

				INLINE bool operator==(const FrameBufferTargets& other) const
				{
					return 0 == memcmp(&images, &other.images, sizeof(images));
				}

				static INLINE uint32_t CalcHash(const FrameBufferTargets& targets)
				{
					return base::CRC32().append(&targets.images, sizeof(images));
				}
			};

			//---

			class ObjectCache : public IBaseObjectCache
			{
			public:
				ObjectCache(Thread* owner);
				virtual ~ObjectCache();

				//--

				INLINE Thread* owner() const { return (Thread*) IBaseObjectCache::owner(); }

				//--

				virtual IBaseVertexBindingLayout* createOptimalVertexBindingLayout(const base::Array<ShaderVertexStreamMetadata>& streams) override;
				virtual IBaseDescriptorBindingLayout* createOptimalDescriptorBindingLayout(const base::Array<ShaderDescriptorMetadata>& descriptors, const base::Array<ShaderStaticSamplerMetadata>& staticSamplers) override;

				//--

				// remove all cached framebuffers
				void clearFramebuffers();

				// create a frame buffer, may fail but tracking is still created
				GLuint buildFramebuffer(const FrameBufferTargets& targets);

				// notify cache that image is being deleted -> this will release all framebuffers this image is in
				void notifyImageDeleted(GLuint glImage);

				//--

				// create sampler (samplers are never deleted)
				GLuint createSampler(const SamplerState& state);

				//--

			private:	
				struct CachedFramebuffer : public base::NoCopy
				{
					RTTI_DECLARE_POOL(POOL_API_RUNTIME);
					
				public:
					GLuint glFrameBuffer = 0;
					bool configured = false;
					FrameBufferTargets targets;
				};

				struct CachedImageInfo : public base::NoCopy
				{
					RTTI_DECLARE_POOL(POOL_API_RUNTIME);

				public:
					GLuint glImage = 0;
					base::InplaceArray<CachedFramebuffer*, 4> frameBuffers;
				};

				base::HashMap<FrameBufferTargets, CachedFramebuffer*> m_framebufferMap;
				base::HashMap<GLuint, CachedImageInfo*> m_imageMap;

				void linkImage(GLuint image, CachedFramebuffer* ret);
				bool configureFramebuffer(CachedFramebuffer* ret);

				//--

				base::HashMap<SamplerState, GLuint> m_samplerMap;
				
				//--
			};

			//---

		} // gl4
    } // api
} // rendering