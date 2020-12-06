/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4ObjectCache.h"
#include "gl4DownloadArea.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			DownloadArea::DownloadArea(Thread* owner, uint32_t size)
				: IBaseDownloadArea(owner, size)
			{}

			DownloadArea::~DownloadArea()
			{
				if (m_mappedMemoryPtr)
				{
					GL_PROTECT(glUnmapNamedBuffer(m_glBuffer));
					m_mappedMemoryPtr = nullptr;
				}

				if (m_glBuffer)
				{
					GL_PROTECT(glDeleteBuffers(1, &m_glBuffer));
					m_glBuffer = 0;
				}
			}

			void DownloadArea::ensureCreated()
			{
				if (m_glBuffer)
					return;

				// create buffer
				GL_PROTECT(glCreateBuffers(1, &m_glBuffer));

				// label the object
				const base::StringView label = "DownloadArea";
				GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, label.length(), label.data()));

				// setup data with buffer
				GL_PROTECT(glNamedBufferStorage(m_glBuffer, size(), nullptr, GL_CLIENT_STORAGE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT));

				// persistently map the buffer
				GL_PROTECT(m_mappedMemoryPtr = (uint8_t*)glMapNamedBufferRange(m_glBuffer, 0, size(), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_FLUSH_EXPLICIT_BIT));
				ASSERT_EX(m_mappedMemoryPtr != nullptr, "Buffer mapping failed");
			}

			GLuint DownloadArea::resolveBuffer()
			{
				ensureCreated();
				return m_glBuffer;
			}

			void* DownloadArea::resolvePointer()
			{
				ensureCreated();
				return m_mappedMemoryPtr;
			}

			//--

			const void* DownloadArea::retrieveDataPointer_ClientApi()
			{
				return m_mappedMemoryPtr.load();
			}

			//--

		} // gl4
    } // gl4
} // rendering
