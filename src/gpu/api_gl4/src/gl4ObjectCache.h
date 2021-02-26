/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "gpu/api_common/include/apiObjectCache.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

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
		return CRC32().append(&targets.images, sizeof(images));
	}
};

//---

// persistently mapped CPU readable memory used to download data from GPU
class DownloadArea : public NoCopy
{
	RTTI_DECLARE_POOL(POOL_API_BACKING_STORAGE);

public:
    DownloadArea(uint32_t size);
    virtual ~DownloadArea();

	INLINE GLuint buffer() const { return m_glBuffer; }
	INLINE const uint8_t* ptr() const { return m_ptr; }
	INLINE uint32_t size() const { return m_size; }

private:
    GLuint m_glBuffer = 0;

	const uint8_t* m_ptr = nullptr;
	uint32_t m_size = 0;
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

	virtual IBaseVertexBindingLayout* createOptimalVertexBindingLayout(const Array<ShaderVertexStreamMetadata>& streams) override;
	virtual IBaseDescriptorBindingLayout* createOptimalDescriptorBindingLayout(const Array<ShaderDescriptorMetadata>& descriptors, const Array<ShaderStaticSamplerMetadata>& staticSamplers) override;

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

	// allocate download area that can accommodate at least given bytes
	DownloadArea* allocateDownloadArea(uint32_t size);

	// release previously used download area
	void freeDownloadArea(DownloadArea* area);

	//--

private:	
	struct CachedFramebuffer : public IReferencable
	{
		RTTI_DECLARE_POOL(POOL_API_RUNTIME);
					
	public:
		bool configured = false;
		GLuint glFrameBuffer = 0;
		FrameBufferTargets targets;
	};

	struct CachedImageInfo : public NoCopy
	{
		RTTI_DECLARE_POOL(POOL_API_RUNTIME);

	public:
		GLuint glImage = 0;
		InplaceArray<RefPtr<CachedFramebuffer>, 4> frameBuffers;
	};

	HashMap<FrameBufferTargets, RefPtr<CachedFramebuffer>> m_framebufferMap;
	HashMap<GLuint, CachedImageInfo*> m_imageMap;

	void linkImage(GLuint image, CachedFramebuffer* ret);
	bool configureFramebuffer(CachedFramebuffer* ret);

	//--

	Array<DownloadArea*> m_freeDownloadAreas;

	//--

	HashMap<SamplerState, GLuint> m_samplerMap;
				
	//--
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
