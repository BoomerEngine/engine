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
    // known size of the header + object data, usually known only if loading from a package, this will save us a read
    uint64_t knownMainFileSize = 0;

    // load path of this resource
    StringBuf resourceLoadPath;

    // expected main class (can save lot's or work)
    ClassType expectedRootClass;

    // load imports (other referenced resources)
    bool loadImports = true;
};

//--

// results of loading a file
struct CORE_RESOURCE_API FileLoadingResult
{
    // root objects from the file, usually one for resources but can be many in general case
    Array<ObjectPtr> roots;

    // get loaded root
    template< typename T >
    INLINE RefPtr<T> root() const
    {
        return (roots.size() == 1) ? rtti_cast<T>(roots[0]) : nullptr;
    }
};

//--

// file loading dependency
struct CORE_RESOURCE_API FileLoadingDependency
{
    ResourceID id; // for ID based resources
    StringBuf depotPath; // for depot path based resources (usually relative to the loaded file)

    SpecificClassType<IResource> cls;
    bool loaded = true;

    FileLoadingDependency();
};

//--

// load objects from file, will do few async reads, does not have to load the whole file
CAN_YIELD extern CORE_RESOURCE_API bool LoadFile(IAsyncFileHandle* file, const FileLoadingContext& context, FileLoadingResult& outResult);

// helper function to load objects from memory
CAN_YIELD extern CORE_RESOURCE_API bool LoadFileDependencies(IAsyncFileHandle* file, const FileLoadingContext& context, Array<FileLoadingDependency>& outDependencies);

// load just the file tables
CAN_YIELD extern CORE_RESOURCE_API bool LoadFileTables(IAsyncFileHandle* file, Buffer& tablesData, Array<uint8_t>* outFileHeader = nullptr);

//--

END_BOOMER_NAMESPACE()
