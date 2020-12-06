/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "base/containers/include/queue.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//---

			struct UniformBuffer
			{
				uint8_t* memoryPtr = nullptr;
				GLuint glBuffer = 0;
			};


			//---

			// simple pool for 64KB blocks of uniform data
			class UniformBufferPool : public base::NoCopy
			{
			public:
				UniformBufferPool();
				virtual ~UniformBufferPool();

				INLINE uint32_t bufferSize() const { return m_bufferSize; }
				INLINE uint32_t bufferAlignment() const { return m_bufferAlignment; }

				UniformBuffer allocateBuffer();

				void returnToPool(UniformBuffer buffer);

			private:
				const base::ThreadID m_owningThreadID;

				base::Queue<UniformBuffer> m_freeEntries;
				uint32_t m_bufferSize = 0;
				uint32_t m_bufferAlignment = 0;

				base::HashSet<GLuint> m_allOwnedBuffers;

				//--

				void createBufferBatch(uint32_t count);

				//--
			};

			//---

		} // gl4
    } // api
} // rendering