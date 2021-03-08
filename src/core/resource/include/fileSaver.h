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

/// saving format
enum class FileSaveFormat : uint8_t
{
    // standard binary serialization with extra data for error recovery (skip blocks, etc)
    ProtectedBinaryStream = 0,

    // packed binary serialization data with NO error recovery (small & fast)
    UnprotectedBinaryStream = 1,

    // standard XML serialization 
    XML = 2,
};

//--

/// read context for loading file deserialization
struct CORE_RESOURCE_API FileSavingContext
{
    // format of data
    FileSaveFormat format = FileSaveFormat::ProtectedBinaryStream;

    // should we extract buffers on save ? (final cooking)
    // if set this prevents buffers from getting saved into the resource and they are saved side-by-side instead by the cooker
    // this is a crucial feature for better async loading of cooked content
    bool extractBuffers = false;

    // should be extract dependency information ?
    bool extractDependencies = false;

    // root object to save
    InplaceArray<const IObject*, 1> rootObjects;
};

//--

/// result of file saving
struct CORE_RESOURCE_API FileSavingResult
{
    //---

    // number of objects written
    uint32_t objectCount = 0;

    // total size of object data
    uint64_t objectDataSize = 0;

    // total size of buffer data
    uint64_t bufferDataSize = 0;

    //---

    // extracted buffer data
    struct ExtractedBuffer
    {
        uint64_t uncompressedCRC = 0;
        uint64_t uncompressedSize = 0;
        CompressionType compressionType;
        Buffer compressedData; // compressed buffer data (or not if it did not compress well)
    };

    Array<ExtractedBuffer> extractedBuffers;

    //--

    // extracted dependency
    struct ExtractedDependency
    {
        ResourceID id;
        bool loaded = false; // does this dependency require immediate loading
    };

    Array<ExtractedDependency> extractedDependencies;

    //--
};

//--

// save objects to file
CAN_YIELD extern CORE_RESOURCE_API bool SaveFile(IWriteFileHandle* file, const FileSavingContext& context, FileSavingResult& outResult, IProgressTracker* progress = nullptr);

//--

END_BOOMER_NAMESPACE()
