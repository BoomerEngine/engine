/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/resources/include/resourceMountPoint.h"
#include "base/resources/include/resource.h"

namespace base
{
    namespace cooker
    {

        //--

        /// helper class that can process raw depot files into cooked engine files
        class BASE_COOKING_API Cooker : public NoCopy
        {
        public:
            Cooker(depot::DepotStructure& depot, res::IResourceLoader* dependencyLoader, IProgressTracker* externalProgressTracker = nullptr, bool finalCooker = false);
            ~Cooker();

            //--

            /// underlying depot
            INLINE depot::DepotStructure& depot() const { return m_depot; }

            //--

            /// check if can cook this file at all
            bool canCook(const res::ResourceKey& key, SpecificClassType<res::IResource>& outCookedResourceClass) const;

            /// cook single file, creates a resource with all metadata set
            res::ResourcePtr cook(res::ResourceKey key) const;

            //--

        private:
            //--

            depot::DepotStructure& m_depot;
            res::IResourceLoader* m_loader;

            struct CookableClass
            {
                SpecificClassType<res::IResource> targetResourceClass = nullptr;
                SpecificClassType<res::IResourceCooker> cookerClass = nullptr;
                uint8_t order = 255; // 0-direct cooking 1-cooking from other resource type
            };

            HashMap<StringBuf, Array<CookableClass>> m_cookableClassmap;
            HashMap<StringBuf, SpecificClassType<res::IResource> > m_selfCookableClasses; // text resources that are cooked by serializing them to binary format

            //---

            struct CookingRequest
            {
                res::ResourceKey key;
                base::Array<uint32_t> externalRequests;
                io::AbsolutePath cookedFilePath;
            };

            typedef HashMap<res::ResourceKey, RefWeakPtr<CookingRequest>> TCoookingRequestMap;
            TCoookingRequestMap m_cookingRequestMap;
            Mutex m_cookingRequestMapLock;

            bool m_finalCooker = false;

            IProgressTracker* m_externalProgressTracker = nullptr;

            //--

            void buildClassMap();

            bool findBestCooker(const res::ResourceKey& key, CookableClass& outBestCooker) const;

            res::ResourcePtr cookUsingCooker(res::ResourceKey key, const res::ResourceMountPoint& mountPoint, const CookableClass& recipe) const;
            //Cooker::Result cookFromTextFormat(res::ResourceKey key, const res::ResourceMountPoint& mountPoint) const;


        };

        //--

    } // cooker
} // base