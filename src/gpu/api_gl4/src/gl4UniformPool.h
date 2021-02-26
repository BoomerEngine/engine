/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "core/containers/include/queue.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//---

struct UniformBuffer
{
	uint8_t* memoryPtr = nullptr;
	GLuint glBuffer = 0;
};


//---

// simple pool for 64KB blocks of uniform data
class UniformBufferPool : public NoCopy
{
public:
	UniformBufferPool();
	virtual ~UniformBufferPool();

	INLINE uint32_t bufferSize() const { return m_bufferSize; }
	INLINE uint32_t bufferAlignment() const { return m_bufferAlignment; }

	UniformBuffer allocateBuffer();

	void returnToPool(UniformBuffer buffer);

private:
	const ThreadID m_owningThreadID;

	Queue<UniformBuffer> m_freeEntries;
	uint32_t m_bufferSize = 0;
	uint32_t m_bufferAlignment = 0;

	HashSet<GLuint> m_allOwnedBuffers;

	//--

	void createBufferBatch(uint32_t count);

	//--
};

//---

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
