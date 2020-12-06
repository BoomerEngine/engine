/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "gl4Thread.h"
#include "gl4Buffer.h"
#include "gl4Utils.h"
#include "gl4CopyQueue.h"
#include "gl4DownloadArea.h"

#include "rendering/device/include/renderingDeviceApi.h"

namespace rendering
{
	namespace api
	{
	    namespace gl4
		{
			//--

			static GLuint DetermineBestUsageFlag(const BufferCreationInfo& info)
			{
				GLuint flags = 0;

				if (info.allowDynamicUpdate)
					flags |= GL_DYNAMIC_STORAGE_BIT;

				return flags;
			}

			//--

			Buffer::Buffer(Thread* drv, const BufferCreationInfo &setup)
				: IBaseBuffer(drv, setup)
			{
				m_glUsage = DetermineBestUsageFlag(setup);
			}

			Buffer::~Buffer()
			{
				if (m_glBuffer)
				{
					GL_PROTECT(glDeleteBuffers(1, &m_glBuffer));
					m_glBuffer = 0;
				}
			}

			void Buffer::ensureCreated()
			{
				PC_SCOPE_LVL1(BufferCreate);

				if (m_glBuffer)
					return;

				// create buffer
				GL_PROTECT(glCreateBuffers(1, &m_glBuffer));

				// label the object
				if (setup().label)
					GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBuffer, setup().label.length(), setup().label.c_str()));

				// setup data with buffer
				GL_PROTECT(glNamedBufferStorage(m_glBuffer, setup().size, nullptr, m_glUsage));
			}

			//--

			IBaseBufferView* Buffer::createConstantView_ClientApi(uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				return new BufferUntypedView(owner(), this, setup);
			}

			IBaseBufferView* Buffer::createView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				setup.format = format;
				return new BufferTypedView(owner(), this, setup);
			}

			IBaseBufferView* Buffer::createStructuredView_ClientApi(uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				setup.stride = this->setup().stride;
				setup.structured = true;
				return new BufferUntypedView(owner(), this, setup);
			}

			IBaseBufferView* Buffer::createWritableView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				setup.format = format;
				setup.writable = true;
				return new BufferTypedView(owner(), this, setup);
			}

			IBaseBufferView* Buffer::createWritableStructuredView_ClientApi(uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				setup.stride = this->setup().stride;
				setup.structured = true;
				setup.writable = true;
				return new BufferUntypedView(owner(), this, setup);
			}

			//--

			ResolvedBufferView Buffer::resolve(uint32_t offset, uint32_t size)
			{
				ensureCreated();

				if (size == INDEX_MAX)
					size = setup().size - offset;

				ASSERT_EX(offset < setup().size, "Invalid offset");
				ASSERT_EX(offset + size <= setup().size, "Invalid offset + size");

				ResolvedBufferView ret;
				ret.glBuffer = m_glBuffer;
				ret.offset = offset;
				ret.size = size;
				return ret;
			}

			void Buffer::initializeFromStaging(IBaseCopyQueueStagingArea* baseData)
			{
				PC_SCOPE_LVL1(BufferAsyncCopy);

				ensureCreated();

				const auto* data = static_cast<CopyQueueStagingArea*>(baseData);
				for (const auto& atom : data->writeAtoms)
				{
					GL_PROTECT(glCopyNamedBufferSubData(data->glBuffer, m_glBuffer, atom.internalOffset, 0, atom.targetDataSize));
				}
			}

			void Buffer::updateFromDynamicData(const void* data, uint32_t dataSize, const ResourceCopyRange& range)
			{
				ensureCreated();

				const auto copySize = std::min<uint32_t>(range.buffer.size, dataSize - range.buffer.offset);
				GL_PROTECT(glNamedBufferSubData(m_glBuffer, range.buffer.offset, copySize, data));
			}

			void Buffer::copyFromBuffer(const ResolvedBufferView& view, const ResourceCopyRange& range)
			{
				ensureCreated();

				const auto copySize = std::min<uint32_t>(range.buffer.size, view.size);
				GL_PROTECT(glCopyNamedBufferSubData(view.glBuffer, m_glBuffer, view.offset, range.buffer.offset, copySize));
			}

			void Buffer::downloadIntoArea(IBaseDownloadArea* area, const ResourceCopyRange& range)
			{
				ensureCreated();

				auto glTargetBuffer = static_cast<DownloadArea*>(area)->resolveBuffer();

				const auto copySize = std::min<uint32_t>(range.buffer.size, area->size());
				GL_PROTECT(glCopyNamedBufferSubData(m_glBuffer, glTargetBuffer, range.buffer.offset, 0, copySize));
			}

			void Buffer::copyFromBuffer(IBaseBuffer* sourceBuffer, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange)
			{
				auto view = static_cast<Buffer*>(sourceBuffer)->resolve(sourceRange.buffer.offset, sourceRange.buffer.size);
				copyFromBuffer(view, targetRange);
			}

			void Buffer::copyFromImage(IBaseImage* sourceImage, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange)
			{
				// TODO!
			}

			//--		

			BufferUntypedView::BufferUntypedView(Thread* drv, Buffer* buffer, const Setup& setup)
				: IBaseBufferView(drv, buffer, ObjectType::BufferUntypedView, setup)
			{}

			BufferUntypedView::~BufferUntypedView()
			{}

			ResolvedBufferView BufferUntypedView::resolve()
			{
				ResolvedBufferView ret;
				ret.glBuffer = buffer()->object();
				ret.offset = setup().offset;
				ret.size = setup().size;
				return ret;
			}

			//--		

			BufferTypedView::BufferTypedView(Thread* drv, Buffer* buffer, const Setup& setup)
				: IBaseBufferView(drv, buffer, ObjectType::BufferTypedView, setup)
			{
				DEBUG_CHECK_EX(setup.format != ImageFormat::UNKNOWN, "Trying to resolve a typed view without specifying a type");
				m_glBufferFormat = TranslateImageFormat(setup.format);
				ASSERT_EX(m_glBufferFormat != 0, "Invalid format for a typed buffer view");
			}

			BufferTypedView::~BufferTypedView()
			{
				if (m_glBufferView)
				{
					GL_PROTECT(glDeleteTextures(1, &m_glBufferView));
					m_glBufferView = 0;
				}
			}

			void BufferTypedView::ensureCreated()
			{
				if (m_glBufferView)
					return;

				if (auto glBuffer = buffer()->object())
				{
					GL_PROTECT(glCreateTextures(GL_TEXTURE_BUFFER, 1, &m_glBufferView));
					GL_PROTECT(glTextureBufferRange(m_glBufferView, m_glBufferFormat, glBuffer, setup().offset, setup().size));
					GL_PROTECT(glObjectLabel(GL_BUFFER, m_glBufferView, -1, base::TempString("TypedView {} @{} of {}", setup().format, setup().offset, buffer()->setup().label).c_str()));
				}
			}

			ResolvedFormatedView BufferTypedView::resolve()
			{
				ensureCreated();

				ResolvedFormatedView ret;
				ret.glBufferView = m_glBufferView;
				ret.glViewFormat = m_glBufferFormat;
				ret.size = setup().size;
				return ret;
			}

			//--

		} // gl4
    } // api
} // rendering
