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

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

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

	m_freeDownloadAreas.clear();
}

IBaseVertexBindingLayout* ObjectCache::createOptimalVertexBindingLayout(const Array<ShaderVertexStreamMetadata>& streams)
{
	return new VertexBindingLayout(owner(), streams);
}

IBaseDescriptorBindingLayout* ObjectCache::createOptimalDescriptorBindingLayout(const Array<ShaderDescriptorMetadata>& descriptors, const Array<ShaderStaticSamplerMetadata>& staticSamplers)
{
	return new DescriptorBindingLayout(owner(), descriptors, staticSamplers);
}

//---

void ObjectCache::clearFramebuffers()
{
	for (const auto& fb : m_framebufferMap.values())
	{
		if (fb->glFrameBuffer)
		{
			GL_PROTECT(glDeleteFramebuffers(1, &fb->glFrameBuffer));
			mem::PoolStats::GetInstance().notifyFree(POOL_API_FRAMEBUFFERS, 1);
		}
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
			m_imageMap[glImage] = ret;
		}

		ret->frameBuffers.pushBack(AddRef(fb));
		TRACE_SPAM("Bound frame buffer image {} to frame buffer {}", glImage, fb->glFrameBuffer);
	}
}

GLuint ObjectCache::buildFramebuffer(const FrameBufferTargets& targets)
{
	// reuse if possible (that's the whole point)
	RefPtr<CachedFramebuffer> ret;
	if (m_framebufferMap.find(targets, ret))
		return configureFramebuffer(ret) ? ret->glFrameBuffer : 0;

	// create frame buffer
	GLuint frameBuffer = 0;
	GL_PROTECT(glCreateFramebuffers(1, &frameBuffer));
	if (!frameBuffer)
		return 0;

	// create cache entry
	ret = RefNew<CachedFramebuffer>();
	ret->targets = targets;
	ret->glFrameBuffer = frameBuffer;
	m_framebufferMap[targets] = ret;

	TRACE_SPAM("Created frame buffer {}", frameBuffer);
	mem::PoolStats::GetInstance().notifyAllocation(POOL_API_FRAMEBUFFERS, 1);

	// bind used images to the FB
	for (auto target : targets.images)
		linkImage(target.glImageView, ret);

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

static const char* TranslateFramebufferError(GLenum error)
{
	switch (error)
	{
#define TEST(x) case x: return #x;
		TEST(GL_FRAMEBUFFER_UNDEFINED)
		TEST(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
		TEST(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
		TEST(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER);
		TEST(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER);
		TEST(GL_FRAMEBUFFER_UNSUPPORTED);
		TEST(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
		TEST(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS);
	}
#undef TEST
		return "Unknown";
}

bool ObjectCache::configureFramebuffer(CachedFramebuffer* ret)
{
	if (ret->configured)
		return true;

	static FastRandState rnd;

	InplaceArray<GLenum, 10> buffers;
	for (uint32_t i = 0; i < FrameBufferTargets::MAX_FRAMEBUFFER_TARGETS; ++i)
	{
		const auto& image = ret->targets.images[i];
		if (i && !image.glImageView)
			break;

		auto glAttachmentType = FrameBufferTargets::glAttachmentNames[i];

		if (image.glViewType == GL_RENDERBUFFER)
		{
			GL_PROTECT(glNamedFramebufferRenderbuffer(ret->glFrameBuffer, glAttachmentType, GL_RENDERBUFFER, image.glImageView));
		}
		else if (image.glViewType == GL_TEXTURE_2D)
		{
			GL_PROTECT(glNamedFramebufferTexture(ret->glFrameBuffer, glAttachmentType, image.glImageView, image.firstMip));
		}
		else if (image.glViewType == GL_TEXTURE_2D_ARRAY)
		{
			GL_PROTECT(glNamedFramebufferTextureLayer(ret->glFrameBuffer, glAttachmentType, image.glImageView, image.firstMip, image.firstSlice));
		}
		else if (image.glViewType == GL_TEXTURE_2D_MULTISAMPLE)
		{
			GL_PROTECT(glNamedFramebufferTexture(ret->glFrameBuffer, glAttachmentType, image.glImageView, image.firstMip));
		}

		if (i >= 1)
			buffers.pushBack(glAttachmentType);
	}

	// specify color outputs
	GL_PROTECT(glNamedFramebufferDrawBuffers(ret->glFrameBuffer, buffers.size(), buffers.typedData()));

	// check the frame buffer
	GLenum status = glCheckNamedFramebufferStatus(ret->glFrameBuffer, GL_FRAMEBUFFER);
	DEBUG_CHECK_RETURN_EX_V(status == GL_FRAMEBUFFER_COMPLETE,
		TempString("Framebuffer setup is not complete, error: {}", TranslateFramebufferError(status)), false);

	// save as configured
	ret->configured = true;
	return true;
}

void ObjectCache::notifyImageDeleted(GLuint glImage)
{
	// if we were tracking that image then delete all frame buffers it's in
	CachedImageInfo* ret = nullptr;
	if (m_imageMap.find(glImage, ret))
	{
		ASSERT(ret->glImage == glImage);

		for (auto fb : ret->frameBuffers)
		{
			if (fb->glFrameBuffer)
			{
				GL_PROTECT(glDeleteFramebuffers(1, &fb->glFrameBuffer));
				mem::PoolStats::GetInstance().notifyFree(POOL_API_FRAMEBUFFERS, 1);
			}

			fb->glFrameBuffer = 0;
			fb->configured = false;

			m_framebufferMap.remove(fb->targets);
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

static Vector4 TranslateBorderColor(const BorderColor mode)
{
	switch (mode)
	{
		case BorderColor::FloatTransparentBlack: return Vector4(0.0f, 0.0f, 0.0f, 0.0f);
		case BorderColor::FloatOpaqueBlack: return Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		case BorderColor::FloatOpaqueWhite: return Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		case BorderColor::IntTransparentBlack: return Vector4(0.0f, 0.0f, 0.0f, 0.0f);
		case BorderColor::IntOpaqueBlack: return Vector4(0.0f, 0.0f, 0.0f, 0.0f);
		case BorderColor::IntOpaqueWhite: return Vector4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	FATAL_ERROR("Invalid mipmap filter mode");
	return Vector4(0.0f, 0.0f, 0.0f, 0.0f);
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

DownloadArea::DownloadArea(uint32_t size)
	: m_size(size)
{
	// create buffer
	GL_PROTECT(glCreateBuffers(1, &m_glBuffer));

	// label the object
	const StringView label = "DownloadArea";
	GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, label.length(), label.data()));

	// setup data with buffer
	GL_PROTECT(glNamedBufferStorage(m_glBuffer, m_size, nullptr, GL_CLIENT_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_COHERENT_BIT));

	// persistently map the buffer
	GL_PROTECT(m_ptr = (uint8_t*)glMapNamedBufferRange(m_glBuffer, 0, m_size, GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));
	ASSERT_EX(m_ptr != nullptr, "Buffer mapping failed");
}

DownloadArea::~DownloadArea()
{
    if (m_ptr)
    {
		GL_PROTECT(glUnmapNamedBuffer(m_glBuffer));
		m_ptr = nullptr;
    }

    if (m_glBuffer)
    {
        GL_PROTECT(glDeleteBuffers(1, &m_glBuffer));
        m_glBuffer = 0;
    }
}


//--

DownloadArea* ObjectCache::allocateDownloadArea(uint32_t size)
{
	int bestAreaIndex = -1;
	DownloadArea* bestArea = nullptr;
	uint32_t bestAreaSize = 0;

	for (auto index : m_freeDownloadAreas.indexRange())
	{
		auto* area = m_freeDownloadAreas[index];
		if (area->size() >= size)
		{
			if (!bestArea || area->size() < bestAreaSize)
			{
				bestAreaIndex = index;
				bestAreaSize = area->size();
				bestArea = area;
			}
		}
	}

	if (bestArea)
	{
		m_freeDownloadAreas.erase(bestAreaIndex);
		return bestArea;
	}

	const auto alignedSize = Align<uint32_t>(size, 256 << 10);
	return new DownloadArea(alignedSize);
}

void ObjectCache::freeDownloadArea(DownloadArea* area)
{
	m_freeDownloadAreas.pushBack(area);
}

//-

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
