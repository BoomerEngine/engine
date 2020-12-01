/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "rendering/api_common/include/apiTransientBuffer.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//---

			class TransientBufferPool;

			class TransientBuffer : public IBaseTransientBuffer
			{
			public:
				TransientBuffer(TransientBufferPool* owner, uint32_t size, TransientBufferType type);
				virtual ~TransientBuffer();

				//--

				virtual void copyDataFrom(const IBaseTransientBuffer* srcBuffer, uint32_t srcOffset, uint32_t destOffset, uint32_t size) override final;
				virtual void flushInnerWrites(uint32_t offset, uint32_t size) override final;

				//--

				ResolvedBufferView resolve(uint32_t offset, uint32_t size) const;

				//--

			private:
				GLuint m_glBuffer = 0;
			};

			//---

			class TransientBufferPool : public IBaseTransientBufferPool
			{
			public:
				TransientBufferPool(IBaseThread* owner, TransientBufferType type);
				virtual ~TransientBufferPool();

			protected:
				virtual IBaseTransientBuffer* createBuffer(uint32_t size) override final;
			};

			//---

		} // gl4
    } // api
} // rendering