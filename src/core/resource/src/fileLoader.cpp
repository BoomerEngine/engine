/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/
#include "build.h"
#include "resource.h"
#include "fileTables.h"
#include "fileLoader.h"
#include "fileCustomLoader.h"

#include "core/io/include/asyncFileHandle.h"
#include "core/object/include/serializationReader.h"
#include "core/object/include/object.h"
#include "core/xml/include/xmlDocument.h"

BEGIN_BOOMER_NAMESPACE()

//--

void ResolveStringIDs(const FileTables& tables, const FileLoadingContext& context, SerializationResolvedReferences& resolvedReferences)
{
    const auto numStringIds = tables.chunkCount(FileTables::ChunkType::Names);
    resolvedReferences.stringIds.resize(numStringIds);

    const auto* strings = tables.stringTable();
    const auto* ptr = tables.nameTable();
    for (uint32_t i = 0; i < numStringIds; ++i, ++ptr)
    {
        const auto* str = strings + ptr->stringIndex;
        //TRACE_INFO("Name[{}]: '{}'", i, str);
        auto stringID = StringID(str);
        resolvedReferences.stringIds[i] = stringID;
    }
}

void ResolveTypes(const FileTables& tables, const FileLoadingContext& context, SerializationResolvedReferences& resolvedReferences)
{
    const auto numTypes = tables.chunkCount(FileTables::ChunkType::Types);
    resolvedReferences.types.resize(numTypes);
    resolvedReferences.typeNames.resize(numTypes);

    resolvedReferences.types[0] = Type();
    resolvedReferences.typeNames[0] = StringID();

    const auto* ptr = tables.typeTable();
    for (uint32_t i = 0; i < numTypes; ++i, ++ptr)
    {
        auto typeName = resolvedReferences.stringIds[ptr->nameIndex];
        resolvedReferences.typeNames[i] = typeName;

        if (typeName)
        {
            auto type = RTTI::GetInstance().findType(typeName);
            if (!type)
                TRACE_WARNING("FileLoad: Unknown type '{}' used in serialization. Type may have been removed or renamed. File may load with errors or not a all.", typeName);

            //TRACE_INFO("Type[{}]: '{}'", i, type);
            resolvedReferences.types[i] = type;
        }
    }
}

void ResolveProperties(const FileTables& tables, const FileLoadingContext& context, SerializationResolvedReferences& resolvedReferences)
{
    const auto numProperties = tables.chunkCount(FileTables::ChunkType::Properties);
    resolvedReferences.properties.resize(numProperties);
    resolvedReferences.propertyNames.resize(numProperties);

    const auto* ptr = tables.propertyTable();
    for (uint32_t i = 0; i < numProperties; ++i, ++ptr)
    {
        auto classType = resolvedReferences.types[ptr->classTypeIndex].toClass();
        auto propertyName = resolvedReferences.stringIds[ptr->nameIndex];

        const Property* prop = nullptr;
        if (classType)
        {
            prop = classType->findProperty(propertyName);
            if (!prop)
            {
                TRACE_WARNING("FileLoad: Missing property '{}' form type '{}' used in serialization. Property may have been removed or renamed. File may load with some small errors.", propertyName, classType->name());
            }
        }

        resolvedReferences.properties[i] = prop;
        resolvedReferences.propertyNames[i] = propertyName;
    }
}

void ResolveImports(const FileTables& tables, const FileLoadingContext& context, SerializationResolvedReferences& resolvedReferences)
{
    if (tables.header()->version < VER_NEW_RESOURCE_ID)
        return;

    const auto numImports = tables.chunkCount(FileTables::ChunkType::Imports);
    resolvedReferences.resources.resize(numImports);

    Array<uint32_t> resourcesToLoad;
    resourcesToLoad.reserve(numImports);

    const auto* ptr = tables.importTable();
    for (uint32_t i = 0; i < numImports; ++i, ++ptr)
    {
        auto classType = resolvedReferences.types[ptr->classTypeIndex].toClass();

        resolvedReferences.resources[i].type = classType;
        resolvedReferences.resources[i].id = GUID(ptr->guid[0], ptr->guid[1], ptr->guid[2], ptr->guid[3]);

        // load ?
        auto flagLoad = (0 != (ptr->flags & FileTables::ImportFlag_Load));
        if (context.loadImports && flagLoad)
            resourcesToLoad.pushBack(i);
    }

    // load the imports
    if (context.loadImports && !resourcesToLoad.empty())
    {
        auto allLoadedSignal = CreateFence("WaitForImports", resourcesToLoad.size());
        for (auto index : resourcesToLoad)
        {
            auto& entry = resolvedReferences.resources[index];

            RunChildFiber("LoadImport") << [&entry, &context, &allLoadedSignal](FIBER_FUNC)
            {
                entry.loaded = LoadResource(entry.id, entry.type);
                SignalFence(allLoadedSignal);
            };
        }

        WaitForFence(allLoadedSignal);
    }
}

void ResolveExports(const FileTables& tables, const FileLoadingContext& context, FileLoadingResult& outResult, SerializationResolvedReferences& resolvedReferences)
{
    const auto numExports = tables.chunkCount(FileTables::ChunkType::Exports);
    resolvedReferences.objects.resize(numExports);

    HashSet<int> warnedClasses;

    const auto* ptr = tables.exportTable();
    for (uint32_t i = 0; i < numExports; ++i, ++ptr)
    {
        auto classTypeName = resolvedReferences.typeNames[ptr->classTypeIndex];
        auto classType = resolvedReferences.types[ptr->classTypeIndex].toClass();

        if (!classType || !classType->is<IObject>() || classType->isAbstract())
        {
            TRACE_WARNING("Object '{}' is using invalid type '{}' that is not a creatable class", i, classTypeName);
            continue;
        }

        ObjectPtr parentObject;
        if (ptr->parentIndex != 0)
        {
            parentObject = resolvedReferences.objects[ptr->parentIndex - 1];
            if (!parentObject) // skip loading if parent object failed to load
                continue;
        }

        auto obj = classType->create<IObject>();
        resolvedReferences.objects[i] = obj;

        if (i == 0 && context.resourceLoadPath)
            if (auto* resource = rtti_cast<IResource>(obj.get()))
                resource->bindLoadPath(context.resourceLoadPath);
    
        if (parentObject)
            obj->parent(parentObject);
        else
            outResult.roots.pushBack(obj);
    }
}

void ResolveReferences(const FileTables& tables, const FileLoadingContext& context, FileLoadingResult& outResult, SerializationResolvedReferences& resolvedReferences)
{
    ResolveStringIDs(tables, context, resolvedReferences);
    ResolveTypes(tables, context, resolvedReferences);
    ResolveProperties(tables, context, resolvedReferences);
    ResolveImports(tables, context, resolvedReferences);
    ResolveExports(tables, context, outResult, resolvedReferences);
}

//--
static const uint64_t DefaultLoadBufferSize = 8U << 20;
static const uint64_t BlockSize = 4096;

uint64_t DetermineLoadBufferSize(IAsyncFileHandle* file, const FileTables& tables, const FileLoadingContext& context, const SerializationResolvedReferences& resolvedReferences)
{
    // whole file is smaller than the load buffer
    // NOTE: use this ONLY if we indeed want to load the whole file
    if (tables.header()->objectsEnd <= DefaultLoadBufferSize)
        return tables.header()->objectsEnd;

    // file is bigger than the load buffer - we can't load it all at once
    // we will load it in chunks but we can't have smaller chunk than the biggest object
    uint64_t maxObjectSize = 0;
    const auto* ptr = tables.exportTable();
    const auto numObjects = tables.chunkCount(FileTables::ChunkType::Exports);
    for (uint32_t i = 0; i < numObjects; ++i, ++ptr)
    {
        // do not consider objects that were disabled from loading
        if (!resolvedReferences.objects[i])
            continue;

        // get size of the object to load
        const auto objectDataSize = ptr->dataSize;
        if (objectDataSize > maxObjectSize)
            maxObjectSize = objectDataSize;
    }

    // account for misalignment
    maxObjectSize += BlockSize;
    return std::max<uint32_t>(maxObjectSize, DefaultLoadBufferSize);
}

bool LoadFileObjects(IAsyncFileHandle* file, const FileTables& tables, const FileLoadingContext& context, FileLoadingResult& outResult)
{
    // do we have "safe layout" in the file ?
    const bool protectedFileLayout = 0 != (tables.header()->flags & FileTables::FileFlag_ProtectedLayout);

    // resolve all references
    // NOTE: this will also create all objects that we want to load
    SerializationResolvedReferences resolvedReferences;
    ResolveReferences(tables, context, outResult, resolvedReferences);

    // get the size of the load buffer and allocate it
    const auto loadBufferSize = DetermineLoadBufferSize(file, tables, context, resolvedReferences);
    const auto loadBuffer = Buffer::Create(POOL_SERIALIZATION, loadBufferSize, 16);
    if (!loadBuffer)
    {
        TRACE_WARNING("LoadFile: Unable to allocate loading buffer of size {}", MemSize(loadBufferSize));
        return false;
    }

    // load data and process it
    const auto* objectTable = tables.exportTable();
    const auto numObjects = tables.chunkCount(FileTables::ChunkType::Exports);
    auto objectIndex = 0;
    while (objectIndex < numObjects)
    {
        // do not consider objects that were disabled from loading
        if (!resolvedReferences.objects[objectIndex])
        {
            objectIndex += 1;
            continue;
        }

        // find the load batch size
        uint32_t firstLoadObject = objectIndex;
        uint64_t batchStartOffset = (objectTable[objectIndex].dataOffset / BlockSize) * BlockSize; // NOTE: aligned to block size (4K)
        uint64_t batchEndOffset = objectTable[objectIndex].dataOffset + objectTable[objectIndex].dataSize;
        objectIndex += 1; // we load at least one

        // but do we fit more ?
        while (objectIndex < numObjects)
        {
            // do not consider objects that were disabled from loading
            if (resolvedReferences.objects[objectIndex])
            {
                const auto currentLoadEnd = objectTable[objectIndex].dataOffset + objectTable[objectIndex].dataSize;
                if (currentLoadEnd > batchStartOffset + loadBufferSize)
                    break; // it won't fit
                batchEndOffset = currentLoadEnd;
            }

            objectIndex += 1;
        }

        // load the data
        const auto loadSize = batchEndOffset - batchStartOffset;
        ASSERT_EX(loadSize <= loadBufferSize, "Load size greated than load buffer");
        const auto loadSizeActual = file->readAsync(batchStartOffset, loadSize, loadBuffer.data());
        if (loadSizeActual != loadSize)
        {
            TRACE_WARNING("LoadFile: AsyncIO failure, loaded {}, expected {} at offset {}", loadSizeActual, loadSize, batchStartOffset);
            return false; // cancels everything
        }

        // deserialize objects that were loaded in that batch
        // TODO: technically this can overlap waiting for the next batch

        for (uint32_t i = firstLoadObject; i < objectIndex; ++i)
        {
            if (auto object = resolvedReferences.objects[i])
            {
                // get the memory range in the loaded buffer where the object content is
                const auto& objectEntry = objectTable[i];
                const auto objectOffsetInBuffer = (objectEntry.dataOffset - batchStartOffset);
                const auto objectDataSize = objectEntry.dataSize;
                        
                // get object data in the buffer
                const auto* objectData = loadBuffer.data() + objectOffsetInBuffer;

                // invalid data loaded ?
                if (protectedFileLayout)
                {
                    const auto crc = CRC32().append(objectData, objectDataSize).crc();
                    if (crc != objectEntry.crc)
                    {
                        TRACE_WARNING("LoadFile: Invalid CRC for object {} ({} != {})", i, crc, objectEntry.crc);
                        return false;
                    }
                }

                // read the crap
                SerializationReader reader(resolvedReferences, objectData, objectDataSize, protectedFileLayout, tables.header()->version);
                object->onReadBinary(reader);
            }
        }
    }

    // post load objects
    for (uint32_t i=0; i<numObjects; ++i)
    {
        if (auto obj = resolvedReferences.objects[i])
            obj->onPostLoad();
    }

    // loaded
    return true;
}

bool LoadFileTables(IAsyncFileHandle* file, Buffer& tablesData, Array<uint8_t>* outFileHeader/*= nullptr*/)
{
    // no file
    if (!file)
        return false;

    // load the header's worth of data
    FileTables::Header header;
    {
        const auto numRead = file->readAsync(0, sizeof(header), &header);
        if (!FileTables::ValidateHeader(header, numRead))
        {
            // export the loaded data so we can detect other data formats
            if (outFileHeader)
            {
                outFileHeader->resize(numRead);
                memcpy(outFileHeader->data(), &header, numRead);
            }

            TRACE_WARNING("LoadFile: Failed to validate header");
            return false;
        }
    }

    // REALLY big ?
    const auto tablesMaxSize = 64UL << 20;
    if (header.headersEnd > tablesMaxSize || header.headersEnd < sizeof(header))
    {
        TRACE_WARNING("LoadFile: Invalid size of file tables ({})", header.headersEnd);
        return false;
    }

    // allocate the data for file tables
    tablesData = Buffer::Create(POOL_SERIALIZATION, header.headersEnd);
    if (!tablesData)
    {
        TRACE_WARNING("LoadFile: Failed to create buffer for file tables (size: {})", header.headersEnd);
        return false;
    }

    // load the file tables
    if (file->readAsync(0, tablesData.size(), tablesData.data()) != tablesData.size())
    {
        TRACE_WARNING("LoadFile: Failed to load file tables");
        return false;
    }

    // validate the file tables
    const auto* tables = (const FileTables*)tablesData.data();
    if (!tables->validate(tablesData.size()))
    {
        TRACE_WARNING("LoadFile: Failed to validate file tables");
        return false;
    }

    // loaded
    return true;
}

bool LoadFile(IAsyncFileHandle* file, const FileLoadingContext& context, FileLoadingResult& outResult)
{
    // try binary format first
    Buffer tablesData;
    InplaceArray<uint8_t, sizeof(FileTables::Header)> loadedHeader;
    if (LoadFileTables(file, tablesData, &loadedHeader))
    {
        const auto& tables = *(FileTables*)tablesData.data();
        return LoadFileObjects(file, tables, context, outResult);
    }

    // try the XML format
    const auto maxCompare = std::min<uint32_t>(strlen(xml::IDocument::HEADER_TEXT()), sizeof(FileTables::Header));
    if (loadedHeader.size() >= maxCompare)
    {
        // try to load from XML!
    }

    // if we are loading with known depot path then try the custom loader
    if (context.resourceLoadPath)
    {
        const auto ext = context.resourceLoadPath.view().lastExtension();
        if (const auto customLoaderClass = FindCustomLoaderForFileExtension(ext))
        {
            if (auto customLoader = customLoaderClass->create<ICustomFileLoader>())
                return customLoader->loadContent(file, context, outResult);
        }
    }

    // failed to load
    return false;
}

//--

FileLoadingDependency::FileLoadingDependency()
{}

bool LoadFileDependencies(IAsyncFileHandle* file, const FileLoadingContext& context, Array<FileLoadingDependency>& outDependencies)
{
    // load file tables
    Buffer tablesData;
    InplaceArray<uint8_t, sizeof(FileTables::Header)> loadedHeader;
    if (LoadFileTables(file, tablesData, &loadedHeader))
    {
        // resolve table data
        const auto& tables = *(FileTables*)tablesData.data();
        SerializationResolvedReferences resolvedReferences;
        ResolveStringIDs(tables, context, resolvedReferences);
        ResolveTypes(tables, context, resolvedReferences);
        ResolveImports(tables, context, resolvedReferences);

        // list imports
        for (const auto& info : resolvedReferences.resources)
        {
            if (const auto resourceClass = info.type.cast<IResource>())
            {
                auto& outEntry = outDependencies.emplaceBack();
                outEntry.id = info.id;
                outEntry.cls = info.type.cast<IResource>();
                outEntry.loaded = info.loaded;
            }
        }
    }

    // try the XML format
    const auto maxCompare = std::min<uint32_t>(strlen(xml::IDocument::HEADER_TEXT()), sizeof(FileTables::Header));
    if (loadedHeader.size() >= maxCompare)
    {
        // try to load from XML!
    }

    // if we are loading with known depot path then try the custom loader
    if (context.resourceLoadPath)
    {
        const auto ext = context.resourceLoadPath.view().lastExtension();
        if (const auto customLoaderClass = FindCustomLoaderForFileExtension(ext))
        {
            if (auto customLoader = customLoaderClass->create<ICustomFileLoader>())
                return customLoader->loadDependencies(file, context, outDependencies);
        }
    }

    return true;
}

//--

END_BOOMER_NAMESPACE()
