/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiDownloadArea.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{
			///---

			// persistently mapped CPU readable memory
			class DownloadArea : public IBaseDownloadArea
			{
			public:
				DownloadArea(Thread* owner, uint32_t size);
				virtual ~DownloadArea();

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

				GLuint resolveBuffer();

				void* resolvePointer();

			private:
				GLuint m_glBuffer = 0;
				std::atomic<void*> m_mappedMemoryPtr = nullptr;

				void ensureCreated();

				//--

				virtual const void* retrieveDataPointer_ClientApi() override final;
			};

			//--

		} // gl4
    } // api
} // rendering

