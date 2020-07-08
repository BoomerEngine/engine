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

#include "resourceLoader.h"
#include "resourceLoaderCached.h"

namespace base
{
    namespace res
    {

        //---

        /// Resource loader for final content, no reloading, no cooking
        class BASE_RESOURCE_API ResourceLoaderFinal : public IResourceLoaderCached
        {
            RTTI_DECLARE_VIRTUAL_CLASS(ResourceLoaderFinal, IResourceLoaderCached);

        public:
            virtual ~ResourceLoaderFinal();

        protected:
            // IResourceLoader
            virtual bool initialize(const app::CommandLine& cmdLine) override final;
            virtual void update()  override final;

            // IResourceLoaderCached
            virtual ResourceHandle loadResourceOnce(const ResourceKey& key) CAN_YIELD override final;

            //--

            struct LoadingEntry
            {
                SpecificClassType<IResource> cls;
                StringBuf extension;
            };

            io::AbsolutePath m_looseFileDir; // in case of loose cooked files
            HashMap<SpecificClassType<IResource>, Array<LoadingEntry>> m_loadingExtensionsMap;

            bool buildLoadingExtensionMap();
            bool assembleCookedFilePath(const ResourceKey& key, io::AbsolutePath& outPath) const;

            StringView<char> findCookedExtension(const ResourceKey& key) const;
        };

        //-----

    } // res
} // base