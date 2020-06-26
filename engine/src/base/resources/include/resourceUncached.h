/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\utils #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "base/resources/include/resourceMountPoint.h"

namespace base
{
    namespace res
    {

        /// load UNCACHED resource from given ABSOLUTE path
        /// NOTE: unless resource loader is provided this call will not load dependencies
        /// NOTE: the loaded resource has no resource proxy and cannot be linked into resource graphs
        extern BASE_RESOURCES_API CAN_YIELD ResourceHandle LoadUncached(const io::AbsolutePath& filePath, IResourceLoader* dependencyLoader = nullptr, ObjectPtr parent = nullptr);

        /// load UNCACHED resource from given ABSOLUTE path
        /// NOTE: unless resource loader is provided this call will not load dependencies
        /// NOTE: the loaded resource has no resource proxy and cannot be linked into resource graphs
        extern BASE_RESOURCES_API CAN_YIELD ResourceHandle LoadUncached(const io::AbsolutePath& filePath, ClassType resourceClass, IResourceLoader* dependencyLoader = nullptr, ObjectPtr parent = nullptr);

        /// load UNCACHED resource from given file stream
        /// NOTE: unless resource loader is provided this call will not load dependencies
        /// NOTE: the loaded resource has no resource proxy and cannot be linked into resource graphs
        extern BASE_RESOURCES_API CAN_YIELD ResourceHandle LoadUncached(StringView<char> contextName, ClassType resourceClass, stream::IBinaryReader& fileStream, IResourceLoader* dependencyLoader = nullptr, ObjectPtr parent = nullptr, const ResourceMountPoint& mountPoint = ResourceMountPoint());

        /// load UNCACHED resource from given file memory buffer
        /// NOTE: unless resource loader is provided this call will not load dependencies
        /// NOTE: the loaded resource has no resource proxy and cannot be linked into resource graphs
        extern BASE_RESOURCES_API CAN_YIELD ResourceHandle LoadUncached(StringView<char> contextName, ClassType resourceClass, const void* data, uint64_t dataSize, IResourceLoader* dependencyLoader = nullptr, ObjectPtr parent = nullptr, const ResourceMountPoint& mountPoint = ResourceMountPoint());

        /// load first object of given class from the file
        /// NOTE: used to load metadata blobs
        /// NOTE: use with CARE!
        extern BASE_RESOURCES_API CAN_YIELD ObjectPtr LoadSelectiveUncached(const io::AbsolutePath& filePath, ClassType selectiveClassLoad);

        /// save UNCACHED resource directly to given absolute path
        /// NOTE: resource extension should match the resource type
        /// NOTE: save happens to memory first
        extern BASE_RESOURCES_API CAN_YIELD bool SaveUncached(const io::AbsolutePath& filePath, const ResourceHandle& data, const ResourceMountPoint& mountPoint);

        /// save UNCACHED resource directly to a buffer, use provided mount path for rooting the resource paths
        /// NOTE: resource extension should match the resource type
        /// NOTE: save happens to memory first
        extern BASE_RESOURCES_API CAN_YIELD Buffer SaveUncachedToBuffer(const IResource* data, const ResourceMountPoint& mountPoint);

        //--

        /// save UNCACHED object directly into text
        extern BASE_RESOURCES_API CAN_YIELD StringBuf SaveUncachedText(const ObjectPtr& data);

        /// load UNCACHED object directly from text
        extern BASE_RESOURCES_API CAN_YIELD ObjectPtr LoadUncachedText(StringView<char>contextName, StringView<char>xmlText, base::res::IResourceLoader* loader=nullptr);

        //--

        /// save UNCACHED object directly into buffer
        extern BASE_RESOURCES_API CAN_YIELD Buffer SaveUncachedBinary(const ObjectPtr& data);

        /// load UNCACHED object directly from memory bufer
        extern BASE_RESOURCES_API CAN_YIELD ObjectPtr LoadUncachedBinary(StringView<char>contextName, const Buffer& buffer, const ObjectPtr& newParent = nullptr);

        //--

        /// extract all referenced resources from list objects, optionally the asynchronously loaded resources can be included as well
        extern BASE_RESOURCES_API void ExtractReferencedResources(const Array<const IObject*>& objects, bool includeAsyncLoaded, HashSet<res::ResourceKey>& outReferencedResources);

        //--

    } // res
} // base