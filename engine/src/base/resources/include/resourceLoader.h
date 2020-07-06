/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once

#include "base/containers/include/array.h"
#include "base/containers/include/mutableArray.h"

namespace base
{
    namespace depot
    {
        class DepotStructure;
    } // depot

    namespace res
    {
        //-----

        // Listener interface for global resource events, can be used to monitor what is going on inside the loader
        class BASE_RESOURCES_API IResourceLoaderEventListener : public base::NoCopy
        {
        public:
            virtual ~IResourceLoaderEventListener();

            /// resource has started loading
            virtual void onResourceLoading(const ResourceKey& path) {};

            /// resource has has failed loading
            virtual void onResourceFailed(const ResourceKey& path) {};

            /// resource has been loaded
            virtual void onResourceLoaded(const ResourceKey& path, const ResourceHandle& resHandle) {};

            /// resource has been unloaded
            virtual void onResourceUnloaded(const ResourceKey& path) {};

            /// missing baked resource
            virtual void onResourceMissingBakedResource(const ResourceKey& path) {};
        };

        //---

        /// Resource loader - top level wrapper for all loading activities within the resource system
        class BASE_RESOURCES_API IResourceLoader : public IReferencable
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IResourceLoader);

        public:
            virtual ~IResourceLoader();

            //----

            /// initialize loader
            virtual bool initialize(const app::CommandLine& cmdLine) = 0;

            /// Sync update (called from main thread periodically)
            virtual void update() = 0;

            /// Load a resource from specified resource key
            /// NOTE: this function will wait for other jobs and thus yield the active fiber
            /// NOTE: if resource is not found or is invalid a resource stub MAY be returned if allowed, later that stub may get reloaded with valid resource
            virtual ResourceHandle loadResource(const ResourceKey& key) CAN_YIELD = 0;

            /// Try to get already loaded resource if it's already loaded
            /// NOTE: this should be used to optimize calls to async loading functions by first querying if resource exists
            /// NOTE: this ONLY returns fully loaded resources (so if the resource is actually being loaded now it's not returned, as the name of the function states)
            virtual ResourceHandle acquireLoadedResource(const ResourceKey& key) = 0;

            //----

            /// register listener that will get informations about what's happening with resources in this loader
            void attachListener(IResourceLoaderEventListener* listener);

            /// detach previously attached listener
            void dettachListener(IResourceLoaderEventListener* listener);

            //----

            /// Load a resource from specified path, may return NULL resource
            /// NOTE: this function can wait for other jobs and thus yield the active fiber
            template<typename T>
            INLINE CAN_YIELD base::RefPtr<T> loadResource(const ResourcePath& path)
            {
                ResourceKey key(path, T::GetStaticClass());
                return rtti_cast<T>(loadResource(key));
            }

            //----

            /// gain the uncooked depot access, valid only in dev builds that are not using cooked data (ie. editor builds)
            /// NOTE: this is one of the few places when dev-only class is visible in non-dev setting
            virtual depot::DepotStructure* queryUncookedDepot() const;

            ///----

        protected:
            typedef MutableArray<IResourceLoaderEventListener*> TListeners;
            Mutex m_listenersLock;
            TListeners m_listeners;

            //--

            void notifyResourceLoading(const ResourceKey& path);
            void notifyResourceLoaded(const ResourceKey& path, const ResourceHandle& data);
            void notifyResourceFailed(const ResourceKey& path);
            void notifyResourceUnloaded(const ResourceKey& path);
            void notifyMissingBakedResource(const ResourceKey& path);

            virtual void feedListenerWithData(IResourceLoaderEventListener* listener);

            //--

            friend IResource; // for notifyResourceUnloaded
        };

        //-----

    } // res
} // base