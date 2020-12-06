/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiBuffer.h"
#include "gl4Thread.h"

namespace rendering
{
    namespace api
    {
		namespace gl4
		{

			//--

			class BufferUntypedView;
			class BufferTypedView;

			//--

			class Buffer : public IBaseBuffer
			{
			public:
				Buffer(Thread* owner, const BufferCreationInfo& setup);
				virtual ~Buffer();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }
				INLINE GLuint object() { ensureCreated(); return m_glBuffer; }

				//--

				virtual IBaseBufferView* createConstantView_ClientApi(uint32_t offset, uint32_t size) override final;
				virtual IBaseBufferView* createView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size) override final;
				virtual IBaseBufferView* createStructuredView_ClientApi(uint32_t offset, uint32_t size) override final;
				virtual IBaseBufferView* createWritableView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size) override final;
				virtual IBaseBufferView* createWritableStructuredView_ClientApi(uint32_t offset, uint32_t size) override final;

				//--

				virtual void initializeFromStaging(IBaseCopyQueueStagingArea* data) override final;
				virtual void updateFromDynamicData(const void* data, uint32_t dataSize, const ResourceCopyRange& range) override final;
				virtual void downloadIntoArea(IBaseDownloadArea* area, uint32_t offsetInArea, uint32_t sizeInArea, const ResourceCopyRange& range) override final;
				virtual void copyFromBuffer(IBaseBuffer* sourceBuffer, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) override final;
				virtual void copyFromImage(IBaseImage* sourceImage, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) override final;

				void copyFromBuffer(const ResolvedBufferView& view, const ResourceCopyRange& range);

				//--

				ResolvedBufferView resolve(uint32_t offset = 0, uint32_t size = INDEX_MAX);

				//--

			private:
				GLuint m_glUsage = 0;
				GLuint m_glBuffer = 0;

				void ensureCreated();

				friend class BufferUntypedView;
				friend class BufferTypedView;
			};

			//---

			// untyped view of the buffer, can be structured with stride
			class BufferUntypedView : public IBaseBufferView
			{
			public:
				BufferUntypedView(Thread* drv, Buffer* buffer, const Setup& setup);
				virtual ~BufferUntypedView();

				static const auto STATIC_TYPE = ObjectType::BufferUntypedView;

				INLINE Buffer* buffer() const { return static_cast<Buffer*>(IBaseBufferView::buffer()); }

				ResolvedBufferView resolve();
			};

			//---

			// typed (formatted) view of the buffer, 
			class BufferTypedView : public IBaseBufferView
			{
			public:
				BufferTypedView(Thread* drv, Buffer* buffer, const Setup& setup);
				virtual ~BufferTypedView();

				static const auto STATIC_TYPE = ObjectType::BufferTypedView;

				INLINE Buffer* buffer() const { return static_cast<Buffer*>(IBaseBufferView::buffer()); }

				ResolvedFormatedView resolve();

			private:
				GLuint m_glBufferFormat = 0;
				GLuint m_glBufferView = 0;

				void ensureCreated();
			};

			//--

		} // gl4 
    } // api
} // rendering
