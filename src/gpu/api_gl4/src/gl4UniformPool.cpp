/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "gl4UniformPool.h"

#include "core/system/include/thread.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//---
			
ConfigProperty<uint32_t> cvUniformPoolBufferSize("Rendering.GL4", "UniformPoolBufferSize", 65536); // 64KB
ConfigProperty<uint32_t> cvUniformPoolInitialCount("Rendering.GL4", "UniformPoolInitialCount", 64); // 4MB of uniforms - most games fit in this with ~4-5k rendered objects
ConfigProperty<uint32_t> cvUniformPoolResizeCount("Rendering.GL4", "UniformPoolResizeCount", 16); // 1MB of extra uniforms each time we run out of space

//--

UniformBufferPool::UniformBufferPool()
	: m_owningThreadID(GetCurrentThreadID())
{
	GLint maxBufferSize = 0;
	GL_PROTECT(glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxBufferSize));
	TRACE_INFO("GL_MAX_UNIFORM_BLOCK_SIZE: {}", maxBufferSize);

	GLint bufferDataAlignment = 0;
	GL_PROTECT(glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &bufferDataAlignment));
	TRACE_INFO("GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT: {}", bufferDataAlignment);

	m_bufferSize = std::clamp<uint32_t>(Align<uint32_t>(cvUniformPoolBufferSize.get(), bufferDataAlignment), 4096, maxBufferSize);
	m_bufferAlignment = std::max<uint32_t>(16, bufferDataAlignment);
	TRACE_INFO("Uniform buffer pool will use buffers of size {} and alignemnt {}", m_bufferSize, m_bufferAlignment);

	createBufferBatch(cvUniformPoolInitialCount.get());
}

UniformBufferPool::~UniformBufferPool()
{
	DEBUG_CHECK_EX(m_allOwnedBuffers.size() == m_freeEntries.size(), "Not all buffers returned to pool");

	for (const auto id : m_allOwnedBuffers.keys())
		GL_PROTECT(glUnmapNamedBuffer(id));

	GL_PROTECT(glDeleteBuffers(m_allOwnedBuffers.size(), m_allOwnedBuffers.keys().typedData()));

	m_allOwnedBuffers.clear();
	m_freeEntries.clear();
}

UniformBuffer UniformBufferPool::allocateBuffer()
{
	DEBUG_CHECK_RETURN_EX_V(m_owningThreadID == GetCurrentThreadID(), "Calling from wrong thread", UniformBuffer());

	if (m_freeEntries.empty())
	{
		const auto extraAllocCount = std::max<uint32_t>(1, cvUniformPoolResizeCount.get());
		createBufferBatch(extraAllocCount);
	}

	UniformBuffer ret = m_freeEntries.top();
	m_freeEntries.pop();

	PoolNotifyAllocation(POOL_API_DYNAMIC_CONSTANT_BUFFER, m_bufferSize);

	return ret;
}

void UniformBufferPool::returnToPool(UniformBuffer buffer)
{
	DEBUG_CHECK_RETURN_EX(m_owningThreadID == GetCurrentThreadID(), "Calling from wrong thread");
	DEBUG_CHECK_RETURN_EX(m_allOwnedBuffers.contains(buffer.glBuffer), "Freeing buffer that does not belong to the pool");

	PoolNotifyFree(POOL_API_DYNAMIC_CONSTANT_BUFFER, m_bufferSize);

	m_freeEntries.push(buffer);
}

void UniformBufferPool::createBufferBatch(uint32_t count)
{
	PC_SCOPE_LVL1(CreateUniformBufferBatch);

	InplaceArray<GLuint, 32> glBuffers;
	glBuffers.resize(count);

	// create the buffers
	GL_PROTECT(glCreateBuffers(glBuffers.size(), glBuffers.typedData()));

	// initialize them and add to queue
	for (GLuint id : glBuffers)
	{
		GL_PROTECT(glObjectLabel(GL_BUFFER, id, -1, "DynamicUniformBuffer"));

		// setup buffer for typical uniform access
		GLuint usageFlags = GL_CLIENT_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
		GL_PROTECT(glNamedBufferStorage(id, m_bufferSize, nullptr, usageFlags));

		UniformBuffer ret;
		ret.glBuffer = id;
		GL_PROTECT(ret.memoryPtr = (uint8_t*)glMapNamedBufferRange(id, 0, m_bufferSize, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
		m_freeEntries.push(ret);

		m_allOwnedBuffers.insert(id);
	}
}

#if 0

//---

TransientBuffer::TransientBuffer(TransientBufferPool* owner, uint32_t size, TransientBufferType type)
	: IBaseTransientBuffer(owner, size, type)
	, m_glBuffer(0)
{
	// create the buffer
	GL_PROTECT(glCreateBuffers(1, &m_glBuffer));

	// label the buffer
	if (type == TransientBufferType::Constants)
		GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, -1, "TransientConstantsuffer"))
	else if (type == TransientBufferType::Staging)
		GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, -1, "TransientStaging"))
	else if (type == TransientBufferType::Geometry)
		GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, -1, "TransientGeometry"));

	// determine buffer usage flags
	GLuint usageFlags = 0;
	if (type == TransientBufferType::Staging)
		usageFlags |= GL_CLIENT_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT;
	else if (type == TransientBufferType::Geometry)
		usageFlags |= GL_CLIENT_STORAGE_BIT;//GL_DYNAMIC_STORAGE_BIT;

	// setup data with buffer
	GL_PROTECT(glNamedBufferStorage(m_glBuffer, size, nullptr, usageFlags));

	// map the buffer
	if (type == TransientBufferType::Staging)
	{
		GL_PROTECT(m_mappedPtr = (uint8_t*)glMapNamedBufferRange(m_glBuffer, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
		ASSERT_EX(m_mappedPtr != nullptr, "Buffer mapping failed");
	}
				
	m_apiPointer.gl = m_glBuffer;
}

TransientBuffer::~TransientBuffer()
{
	// unmap
	if (m_mappedPtr)
	{
		GL_PROTECT(glUnmapNamedBuffer(m_glBuffer));
		m_mappedPtr = nullptr;
	}

	// destroy buffer
	if (0 != m_glBuffer)
	{
		GL_PROTECT(glDeleteBuffers(1, &m_glBuffer));
		m_glBuffer = 0;
	}
}

void TransientBuffer::copyDataFrom(const IBaseTransientBuffer* srcBuffer, uint32_t srcOffset, uint32_t destOffset, uint32_t size)
{
	const auto* srcTransientBuffer = static_cast<const TransientBuffer*>(srcBuffer);

	ASSERT_EX(m_mappedPtr == nullptr, "Copy can only happen into unmapped buffer");
	ASSERT_EX(srcOffset + size <= srcTransientBuffer->size(), "Out of bounds read");
	ASSERT_EX(destOffset + size <= this->size(), "Out of bounds write");
	GL_PROTECT(glCopyNamedBufferSubData(srcTransientBuffer->m_glBuffer, m_glBuffer, srcOffset, destOffset, size));
}

void TransientBuffer::flushInnerWrites(uint32_t offset, uint32_t size)
{
	GL_PROTECT(glFlushMappedNamedBufferRange(m_glBuffer, offset, size));
}

ResolvedBufferView TransientBuffer::resolve(uint32_t offset, uint32_t size) const
{
	ResolvedBufferView ret;
	ret.glBuffer = m_glBuffer;
	ret.offset = offset;
	ret.size = size;
	return ret;
}

//---

TransientBufferPool::TransientBufferPool(IBaseThread* owner, TransientBufferType type)
	: IBaseTransientBufferPool(owner, type)
{}

TransientBufferPool::~TransientBufferPool()
{}

IBaseTransientBuffer* TransientBufferPool::createBuffer(uint32_t size)
{
	return new TransientBuffer(this, size, type());
}

//---
#endif

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
