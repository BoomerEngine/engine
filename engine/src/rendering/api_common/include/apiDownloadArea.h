/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#pragma once

#include "apiObject.h"

namespace rendering
{
	namespace api
	{

		//---

		/// general download area (staging pool)
		class RENDERING_API_COMMON_API IBaseDownloadArea : public IBaseObject
		{
		public:
			IBaseDownloadArea(IBaseThread* owner, uint32_t size);
			virtual ~IBaseDownloadArea();

			static const auto STATIC_TYPE = ObjectType::DownloadArea;

			//--

			INLINE uint32_t size() const { return m_size; }

			//--

			// retrieve data pointer
			virtual const void* retrieveDataPointer_ClientApi() = 0;

			//--

		private:
			uint32_t m_size;
		};

		//---

		class RENDERING_API_COMMON_API DownloadAreaProxy : public IDownloadAreaObject
		{
		public:
			DownloadAreaProxy(ObjectID id, IDeviceObjectHandler* impl, uint32_t size);

			virtual const uint8_t* memoryPointer() const override final;
		};

		//---


	} // api
} // rendering