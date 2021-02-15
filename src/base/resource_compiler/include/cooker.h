/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: cooking #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/resource/include/resourceCooker.h"

namespace base
{
    namespace res
    {

        //--

        /// helper class that can process raw depot files into cooked engine files
        class BASE_RESOURCE_COMPILER_API Cooker : public NoCopy
        {
        public:
            Cooker(IResourceLoader* dependencyLoader, IProgressTracker* externalProgressTracker = nullptr, bool finalCooker = false);
            ~Cooker();

            //--

            /// check if can cook this file at all
            bool canCook(const ResourceKey& key, SpecificClassType<IResource>& outCookedResourceClass) const;

            /// cook single file, creates a resource with all metadata set
            ResourcePtr cook(ResourceKey key) const;

            //--

        private:
            //--

            IResourceLoader* m_loader;

            struct CookableClass
            {
                SpecificClassType<IResource> targetResourceClass = nullptr;
                SpecificClassType<IResourceCooker> cookerClass = nullptr;
                uint8_t order = 255; // 0-direct cooking 1-cooking from other resource type
            };

            HashMap<StringBuf, Array<CookableClass>> m_cookableClassmap;

            //---

            struct CookingRequest
            {
                ResourceKey key;
                Array<uint32_t> externalRequests;
                StringBuf cookedFilePath;
            };

            typedef HashMap<ResourceKey, RefWeakPtr<CookingRequest>> TCoookingRequestMap;
            TCoookingRequestMap m_cookingRequestMap;
            Mutex m_cookingRequestMapLock;

            bool m_finalCooker = false;

            IProgressTracker* m_externalProgressTracker = nullptr;

            //--

            void buildClassMap();

            bool findBestCooker(const ResourceKey& key, CookableClass& outBestCooker) const;

            ResourcePtr cookUsingCooker(ResourceKey key, const CookableClass& recipe) const;
        };

        //--

    } // res
} // base