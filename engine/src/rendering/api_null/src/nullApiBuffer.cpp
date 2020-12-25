/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api\objects #]
***/

#include "build.h"
#include "nullApiThread.h"
#include "nullApiBuffer.h"

#include "rendering/device/include/renderingDeviceApi.h"

#include "base/memory/include/poolStats.h"

namespace rendering
{
	namespace api
	{
	    namespace nul
		{

			//--

			Buffer::Buffer(Thread* drv, const BufferCreationInfo &setup, const ISourceDataProvider* sourceData)
				: IBaseBuffer(drv, setup, sourceData)
			{
			}

			Buffer::~Buffer()
			{
			}

			IBaseBufferView* Buffer::createConstantView_ClientApi(uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				return new BufferAnyView(owner(), this, setup);
			}

			IBaseBufferView* Buffer::createView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				setup.format = format;
				return new BufferAnyView(owner(), this, setup);
			}

			IBaseBufferView* Buffer::createStructuredView_ClientApi(uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				setup.stride = this->setup().stride;
				setup.structured = true;
				return new BufferAnyView(owner(), this, setup);
			}

			IBaseBufferView* Buffer::createWritableView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				setup.format = format;
				setup.writable = true;
				return new BufferAnyView(owner(), this, setup);
			}

			IBaseBufferView* Buffer::createWritableStructuredView_ClientApi(uint32_t offset, uint32_t size)
			{
				IBaseBufferView::Setup setup;
				setup.offset = offset;
				setup.size = size;
				setup.stride = this->setup().stride;
				setup.structured = true;
				setup.writable = true;
				return new BufferAnyView(owner(), this, setup);
			}

			//--

			void Buffer::updateFromDynamicData(const void* data, uint32_t dataSize, const ResourceCopyRange& range)
			{

			}

			void Buffer::downloadIntoArea(IBaseDownloadArea* area, uint32_t offsetInArea, uint32_t sizeInArea, const ResourceCopyRange& range)
			{

			}

			void Buffer::copyFromBuffer(IBaseBuffer* sourceBuffer, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange)
			{

			}

			void Buffer::copyFromImage(IBaseImage* sourceImage, const ResourceCopyRange& sourceRange, const ResourceCopyRange& targetRange)
			{

			}

			//--		

			BufferAnyView::BufferAnyView(Thread* drv, Buffer* buffer, const Setup& setup)
				: IBaseBufferView(drv, buffer, (setup.format != ImageFormat::UNKNOWN) ? ObjectType::BufferTypedView : ObjectType::BufferUntypedView, setup)
			{}

			BufferAnyView::~BufferAnyView()
			{}

			//--

		} // nul
    } // api
} // rendering
