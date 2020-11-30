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
			{
				m_bufferPtr = (uint8_t*) base::mem::AllocSystemMemory(size, false);

				m_mappedPtr = m_bufferPtr;
				m_apiPointer.dx = m_bufferPtr;
			}

			TransientBuffer::~TransientBuffer()
			{
				if (m_bufferPtr)
				{
					base::mem::FreeSystemMemory(m_bufferPtr, size());
					m_bufferPtr = nullptr;
				}
			}

			void TransientBuffer::copyDataFrom(const IBaseTransientBuffer* srcBuffer, uint32_t srcOffset, uint32_t destOffset, uint32_t size)
			{
				const auto* srcBufferEx = static_cast<const TransientBuffer*>(srcBuffer);
				memcpy(srcBufferEx->m_bufferPtr + srcOffset, m_bufferPtr + destOffset, size);
			}

			void TransientBuffer::flushInnerWrites(uint32_t offset, uint32_t size)
			{

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
