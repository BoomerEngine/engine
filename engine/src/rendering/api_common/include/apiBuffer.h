/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiObject.h"

#include "rendering/device/include/renderingResources.h"
#include "rendering/device/include/renderingBuffer.h"

namespace rendering
{
    namespace api
    {
		
		//--

		// base buffer view
		class RENDERING_API_COMMON_API IBaseBufferView : public IBaseObject
		{
		public:
			struct Setup
			{
				uint32_t offset = 0;
				uint32_t size = 0;
				uint32_t stride = 0;
				bool writable = false;
				bool structured = false;
				ImageFormat format = ImageFormat::UNKNOWN;
			};

			IBaseBufferView(IBaseThread* owner, IBaseBuffer* buffer, ObjectType viewType, const Setup& setup);
			virtual ~IBaseBufferView();

			//--

			INLINE IBaseBuffer* buffer() const { return m_buffer; }
			INLINE const Setup& setup() const { return m_setup; }

			//--

		protected:
			IBaseBuffer* m_buffer = nullptr;

			Setup m_setup;
		};

		//--

		// buffer object
		class RENDERING_API_COMMON_API IBaseBuffer : public IBaseCopiableObject
		{
		public:
			IBaseBuffer(IBaseThread* owner, const BufferCreationInfo& setup, const ISourceDataProvider* initData);
			virtual ~IBaseBuffer();

			static const auto STATIC_TYPE = ObjectType::Buffer;

			//--

			// get original setup
			INLINE const BufferCreationInfo& setup() const { return m_setup; }

			//---

			virtual IBaseBufferView* createConstantView_ClientApi(uint32_t offset, uint32_t size) = 0;
			virtual IBaseBufferView* createView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size) = 0;
			virtual IBaseBufferView* createStructuredView_ClientApi(uint32_t offset, uint32_t size) = 0;
			virtual IBaseBufferView* createWritableView_ClientApi(ImageFormat format, uint32_t offset, uint32_t size) = 0;
			virtual IBaseBufferView* createWritableStructuredView_ClientApi(uint32_t offset, uint32_t size) = 0;

			//--

		protected:
			SourceDataProviderPtr m_initData;

			BufferCreationInfo m_setup;
			PoolTag m_poolTag;
		};

		//---
		
		// client side proxy for buffer object
		class RENDERING_API_COMMON_API BufferObjectProxy : public BufferObject
		{
		public:
			BufferObjectProxy(ObjectID id, IDeviceObjectHandler* impl, const Setup& setup);

			virtual BufferConstantViewPtr createConstantView(uint32_t offset/* = 0*/, uint32_t size /*= INDEX_MAX*/);
			virtual BufferViewPtr createView(ImageFormat format, uint32_t offset, uint32_t size) override;
			virtual BufferStructuredViewPtr createStructuredView(uint32_t offset, uint32_t size) override;
			virtual BufferWritableViewPtr createWritableView(ImageFormat format, uint32_t offset, uint32_t size) override;
			virtual BufferWritableStructuredViewPtr createWritableStructuredView(uint32_t offset, uint32_t size) override;
		};

        //--

    } // api
} // rendering