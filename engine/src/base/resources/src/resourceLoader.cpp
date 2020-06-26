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

        IResourceLoaderEventListener::~IResourceLoaderEventListener()
        {}

        //--

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResourceLoader);
        RTTI_END_TYPE();

        IResourceLoader::~IResourceLoader()
        {
        }

        depot::DepotStructure* IResourceLoader::queryUncookedDepot() const
        {
            return nullptr;
        }

        void IResourceLoader::notifyResourceLoading(const ResourceKey& path)
        {
            auto lock = CreateLock(m_listenersLock);
            for (auto it  : m_listeners)
                it->onResourceLoading(path);
        }

        void IResourceLoader::notifyResourceFailed(const ResourceKey& path)
        {
            auto lock = CreateLock(m_listenersLock);
            for (auto it  : m_listeners)
                it->onResourceFailed(path);
        }

        void IResourceLoader::notifyResourceLoaded(const ResourceKey& path, const ResourceHandle& data)
        {
            auto lock = CreateLock(m_listenersLock);
            for (auto it  : m_listeners)
                it->onResourceLoaded(path, data);
        }

        void IResourceLoader::notifyMissingBakedResource(const ResourceKey& path)
        {
            TRACE_WARNING("Missing baked resource '{}'", path);

            for (auto it : m_listeners)
                it->onResourceMissingBakedResource(path); 
        }

        void IResourceLoader::notifyResourceUnloaded(const ResourceKey& path)
        {
            auto lock = CreateLock(m_listenersLock);
            TRACE_INFO("Unloaded resource '{}'", path);

            for (auto it  : m_listeners)
                it->onResourceUnloaded(path);
        }
        
        void IResourceLoader::attachListener(IResourceLoaderEventListener* listener)
        {
            auto lock = CreateLock(m_listenersLock);
            m_listeners.pushBack(listener);
        }

        void IResourceLoader::dettachListener(IResourceLoaderEventListener* listener)
        {
            auto lock = CreateLock(m_listenersLock);
            m_listeners.remove(listener);
        }

        //---

    } // res
} // base

