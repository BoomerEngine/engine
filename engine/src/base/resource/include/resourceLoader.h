/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once

#include "base/containers/include/array.h"
#include "base/object/include/globalEventKey.h"

namespace base
{
    namespace depot
    {
        class DepotStructure;
    } // depot

    namespace res
    {
        //-----

        /// Resource loader - top level wrapper for all loading activities within the resource system
        class BASE_RESOURCE_API IResourceLoader : public IReferencable
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IResourceLoader);

        public:
            virtual ~IResourceLoader();

            //----

            /// get the event key, we can use the key to observer for events
            INLINE const GlobalEventKey& eventKey() const { return m_eventKey; }

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
            virtual bool acquireLoadedResource(const ResourceKey& key, ResourceHandle& outLoadedPtr) = 0;

            //----

            /// Load a resource from specified path, may return NULL resource
            /// NOTE: this function can wait for other jobs and thus yield the active fiber
            template<typename T>
            INLINE CAN_YIELD base::RefPtr<T> loadResource(StringView path)
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
            IResourceLoader();

            GlobalEventKey m_eventKey;

            //--

            void notifyResourceLoading(const ResourceKey& path);
            void notifyResourceLoaded(const ResourceKey& path, const ResourceHandle& data);
            void notifyResourceFailed(const ResourceKey& path);
            void notifyResourceUnloaded(const ResourceKey& path);

            //--

            friend IResource; // for notifyResourceUnloaded
        };

        //-----

    } // res
} // base