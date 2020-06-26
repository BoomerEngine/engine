/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml #]
***/

#pragma once

#include "base/xml/include/public.h"
#include "base/containers/include/hashMap.h"
#include "base/containers/include/array.h"

namespace base
{
    namespace res
    {
        namespace xml
        {
            namespace prv
            {

                /// helper object to resolve named path references to another resources
                class LoaderReferenceResolver
                {
                public:
                    LoaderReferenceResolver(res::IResourceLoader* loader);

                    // get the resolved object, NOTE: the resource may not be fully loaded yet
                    res::ResourceHandle resolveSyncReference(ClassType resourceClass, const res::ResourcePath& path);

                    // get the loaded bound
                    INLINE res::IResourceLoader* resourceLoader() const { return m_resourceLoader; }

                private:
                    base::Array<res::ResourceHandle> m_resources;
                    res::IResourceLoader* m_resourceLoader;
                };

            } // prv
        } // xml
    } // res
} // base