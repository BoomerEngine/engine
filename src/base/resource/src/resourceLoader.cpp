/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#include "build.h"

#include "resource.h"
#include "resourceLoader.h"

#include "base/system/include/scopeLock.h"

namespace base
{
    namespace res
    {

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceLoader);
        RTTI_END_TYPE();

        IResourceLoader::IResourceLoader()
        {
            m_eventKey = MakeUniqueEventKey("ResourceLoader");
        }

        IResourceLoader::~IResourceLoader()
        {
        }

        void IResourceLoader::notifyResourceLoading(const ResourceKey& path)
        {
            DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_LOADING, path);
        }

        void IResourceLoader::notifyResourceFailed(const ResourceKey& path)
        {
            DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_FAILED, path);
        }

        void IResourceLoader::notifyResourceLoaded(const ResourceKey& path, const ResourceHandle& data)
        {
            DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_LOADED, data);
        }

        void IResourceLoader::notifyResourceUnloaded(const ResourceKey& path)
        {
            DispatchGlobalEvent(m_eventKey, EVENT_RESOURCE_LOADER_FILE_UNLOADED, path);
        }

        //---

    } // res
} // base

