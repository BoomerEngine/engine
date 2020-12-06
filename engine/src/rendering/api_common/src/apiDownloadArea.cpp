/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"
#include "apiDownloadArea.h"

namespace rendering
{
    namespace api
    {
		//--

		IBaseDownloadArea::IBaseDownloadArea(IBaseThread* owner, uint32_t size)
			: IBaseObject(owner, ObjectType::DownloadArea)
			, m_size(size)
		{}

		IBaseDownloadArea::~IBaseDownloadArea()
		{}

		//--

		DownloadAreaProxy::DownloadAreaProxy(ObjectID id, IDeviceObjectHandler* impl, uint32_t size)
			: IDownloadAreaObject(id, impl, size)
		{}

		const uint8_t* DownloadAreaProxy::memoryPointer() const
		{
			if (auto* obj = resolveInternalApiObject<IBaseDownloadArea>())
				return (const uint8_t*) obj->retrieveDataPointer_ClientApi();

			return nullptr;
		}

		//--

    } // api
} // rendering
