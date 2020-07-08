/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/resource/include/resourceMountPoint.h"
#include "base/resource/include/resource.h"

namespace base
{
    namespace res
    {

        //--

        /// helper class that can process raw depot files into cooked engine files
        class BASE_RESOURCE_COMPILER_API Cooker : public NoCopy
        {
        public:
            Cooker(depot::DepotStructure& depot, IResourceLoader* dependencyLoader, IProgressTracker* externalProgressTracker = nullptr, bool finalCooker = false);
            ~Cooker();

            //--

            /// underlying depot
            INLINE depot::DepotStructure& depot() const { return m_depot; }

            //--

            /// check if can cook this file at all
            bool canCook(const ResourceKey& key, SpecificClassType<IResource>& outCookedResourceClass) const;

            /// cook single file, creates a resource with all metadata set
            ResourcePtr cook(ResourceKey key) const;

            //--

        private:
            //--

            depot::DepotStructure& m_depot;
            IResourceLoader* m_loader;

            struct CookableClass
            {
                SpecificClassType<IResource> targetResourceClass = nullptr;
                SpecificClassType<IResourceCooker> cookerClass = nullptr;
                uint8_t order = 255; // 0-direct cooking 1-cooking from other resource type
            };

            HashMap<StringBuf, Array<CookableClass>> m_cookableClassmap;
            HashMap<StringBuf, SpecificClassType<IResource> > m_selfCookableClasses; // text resources that are cooked by serializing them to binary format

            //---

            struct CookingRequest
            {
                ResourceKey key;
                Array<uint32_t> externalRequests;
                io::AbsolutePath cookedFilePath;
            };

            typedef HashMap<ResourceKey, RefWeakPtr<CookingRequest>> TCoookingRequestMap;
            TCoookingRequestMap m_cookingRequestMap;
            Mutex m_cookingRequestMapLock;

            bool m_finalCooker = false;

            IProgressTracker* m_externalProgressTracker = nullptr;

            //--

            void buildClassMap();

            bool findBestCooker(const ResourceKey& key, CookableClass& outBestCooker) const;

            ResourcePtr cookUsingCooker(ResourceKey key, const ResourceMountPoint& mountPoint, const CookableClass& recipe) const;
            //Cooker::Result cookFromTextFormat(ResourceKey key, const ResourceMountPoint& mountPoint) const;


        };

        //--

    } // res
} // base