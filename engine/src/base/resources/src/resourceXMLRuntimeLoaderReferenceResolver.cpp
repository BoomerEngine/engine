/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\xml\loader #]
***/

#include "build.h"
#include "resource.h"
#include "resourceLoader.h"
#include "resourceXMLRuntimeLoaderReferenceResolver.h"

std::atomic<int> GNumWaitingImportTables;

namespace base
{
    namespace res
    {
        namespace xml
        {
            namespace prv
            {

                LoaderReferenceResolver::LoaderReferenceResolver(IResourceLoader* loader)
                    : m_resourceLoader(loader)
                {
                }

                res::ResourceHandle LoaderReferenceResolver::resolveSyncReference(ClassType resourceClass, const res::ResourcePath& path)
                {
                    // no resource loaded specified, nothing will be loaded
                    if (!m_resourceLoader)
                        return nullptr;

                    // no path to load
                    if (path.empty())
                        return nullptr;

                    // request loading of a new resources
                    ResourceKey key(path, resourceClass.cast<IResource>());
                    auto resource = m_resourceLoader->loadResource(key);

                    // keep around
                    if (resource)
                        m_resources.pushBack(resource);

                    // return the resource directly
                    return resource;
                }


            } // prv
        } // xml
    } // res
} // base