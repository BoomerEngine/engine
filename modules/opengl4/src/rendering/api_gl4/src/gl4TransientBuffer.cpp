/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "gl4TransientBuffer.h"

namespace rendering
{
	namespace api
	{
		namespace gl4
		{

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

		} // gl4
	} // api
} // rendering
