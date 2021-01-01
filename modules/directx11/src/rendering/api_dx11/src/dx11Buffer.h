/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#pragma once

#include "rendering/api_common/include/apiBuffer.h"

namespace rendering
{
    namespace api
    {
		namespace dx11
		{

			//--

			class Buffer : public IBaseBuffer
			{
			public:
				Buffer(Thread* owner, const BufferCreationInfo& setup, const ISourceDataProvider* sourceData);
				virtual ~Buffer();

				//--

				INLINE Thread* owner() const { return static_cast<Thread*>(IBaseObject::owner()); }

				//--

				virtual IBaseBufferView* createConstantView_ClientApi(uint32_t offset, uint32_t size) override final;
				virtual IBaseBufferView* createView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size) override final;
				virtual IBaseBufferView* createStructuredView_ClientApi(uint32_t offset, uint32_t size) override final;
				virtual IBaseBufferView* createWritableView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size) override final;
				virtual IBaseBufferView* createWritableStructuredView_ClientApi(uint32_t offset, uint32_t size) override final;

				//--

				virtual void updateFromDynamicData(const void* data, uint32_t dataSize, const ResourceCopyRange& range) override final;
				virtual void copyFromBuffer(IBaseBuffer* sourceBuffer, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) override final;
				virtual void copyFromImage(IBaseImage* sourceImage, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange) override final;

				//--

			private:
				// api ptr
			};

			//---

			// any view of the buffer, usually APIs require more specific views
			class BufferAnyView : public IBaseBufferView
			{
			public:
				BufferAnyView(Thread* drv, Buffer* buffer, const Setup& setup);
				virtual ~BufferAnyView();

			private:
				// api ptr
			};

			//--

		} // dx11 
    } // api
} // rendering
