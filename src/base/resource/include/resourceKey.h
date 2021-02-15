/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/containers/include/hashMap.h"
#include "resourcePath.h"

namespace base
{
    namespace res
    {
        ///---

        /// resource path + resource class
        class BASE_RESOURCE_API ResourceKey
        {
        public:
            INLINE ResourceKey() {};
            INLINE ResourceKey(const ResourceKey& other) = default;
            INLINE ResourceKey(ResourceKey&& other);
            INLINE ResourceKey(const ResourcePath& path, SpecificClassType<IResource> classType);
            INLINE ResourceKey& operator=(const ResourceKey& other) = default;
            INLINE ResourceKey& operator=(ResourceKey&& other);

            INLINE bool operator==(const ResourceKey& other) const;
            INLINE bool operator!=(const ResourceKey& other) const;

            //--

            ALWAYS_INLINE const ResourcePath& path() const;
            ALWAYS_INLINE SpecificClassType<IResource> cls() const;

            INLINE bool empty() const;
            INLINE bool valid() const; // only non empty paths are valid

            INLINE operator bool() const;
            
            //--

            // print in editor format ie. "Texture:engine/textures/lena.png"
            void print(IFormatStream& f) const;

            //--

            // parse from a string, usually does a split near ":" to extract class name vs path
            static bool Parse(StringView path, ResourceKey& outKey);

            // calculate hash for hashmaps
            static uint32_t CalcHash(const ResourceKey& key);

            //--

            /// build unique event key for this path (slow)
            GlobalEventKey buildEventKey() const;

            //--

            // empty key
            static const ResourceKey& EMPTY();

            //--

        private:
            ResourcePath m_path;
            SpecificClassType<IResource> m_class;
        };

    } // res

    //--

} // base

#include "resourceKey.inl"