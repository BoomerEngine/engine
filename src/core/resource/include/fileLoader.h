/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

/// read context for loading file deserialization
struct CORE_RESOURCE_API FileLoadingContext
{
    //--

    // known size of the header + object data, usually known only if loading from a package, this will save us a read
    uint64_t knownMainFileSize = 0;

    // specific object class to load (usually the metadata or the thumbnail)
    ClassType loadSpecificClass;

    // load path of this resource
    StringBuf resourceLoadPath;

    // class override for root object - force changes the root object class but still tries to load properties
    ClassType mutatedRootClass;

    // load imports (other referenced resources)
    bool loadImports = true;

    //--

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
struct CORE_RESOURCE_API FileLoadingDependency
{
    ResourceID id;
    SpecificClassType<IResource> cls;
    bool loaded = true;

    FileLoadingDependency();
};

//--

// load objects from file, will do few async reads, does not have to load the whole file
CAN_YIELD extern CORE_RESOURCE_API bool LoadFile(IAsyncFileHandle* file, FileLoadingContext& context);

// helper function to load objects from memory
CAN_YIELD extern CORE_RESOURCE_API bool LoadFileDependencies(IAsyncFileHandle* file, const FileLoadingContext& context, Array<FileLoadingDependency>& outDependencies);

// load just the file tables
CAN_YIELD extern CORE_RESOURCE_API bool LoadFileTables(IAsyncFileHandle* file, Buffer& tablesData);

// load file's metadata
CAN_YIELD extern CORE_RESOURCE_API ResourceMetadataPtr LoadFileMetadata(IAsyncFileHandle* file, const FileLoadingContext& context);

//--

END_BOOMER_NAMESPACE()
