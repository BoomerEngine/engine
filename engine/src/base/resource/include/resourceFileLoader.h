/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#pragma once

namespace base
{
    namespace res
    {
        //--

        /// read context for loading file deserialization
        struct BASE_RESOURCE_API FileLoadingContext
        {
            //--

            // known size of the header + object data, usually known only if loading from a package, this will save us a read
            uint64_t knownMainFileSize = 0;

            // specific object class to load (usually the metadata or the thumbnail)
            ClassType loadSpecificClass;

            // resource loader to use to load any other dependencies, if not specified the resources will not be loaded (although the keys will remain valid)
            IResourceLoader* resourceLoader = nullptr;

            // base reference path for resource loading
            StringBuf basePath;

            // class override for root object - force changes the root object class but still tries to load properties
            ClassType mutatedRootClass;

            //--

            // ALL loaded object (may be big)
            //Array<ObjectPtr> loadedObjects;

            // all loaded root objects (without parents)
            Array<ObjectPtr> loadedRoots;

            //--

            // get loaded root
            template< typename T >
            INLINE RefPtr<T> root() const
            {
                if (loadedRoots.size() == 1)
                    return rtti_cast<T>(loadedRoots[0]);
                return nullptr;
            }
        };

        //--

        // file loading dependency
        struct BASE_RESOURCE_API FileLoadingDependency
        {
            ResourceKey key;
            bool loaded = true;

            FileLoadingDependency();
        };

        //--

        // load objects from file, will do few async reads, does not have to load the whole file
        CAN_YIELD extern BASE_RESOURCE_API bool LoadFile(io::IAsyncFileHandle* file, FileLoadingContext& context);

        // helper function to load objects from memory
        CAN_YIELD extern BASE_RESOURCE_API bool LoadFileDependencies(io::IAsyncFileHandle* file, const FileLoadingContext& context, Array<FileLoadingDependency>& outDependencies);

        // load file's metadata
        CAN_YIELD extern BASE_RESOURCE_API MetadataPtr LoadFileMetadata(io::IAsyncFileHandle* file, const FileLoadingContext& context);

        //--

    } // res
} // base