/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/
#include "build.h"
#include "resourceMetadata.h"
#include "resourceFileTables.h"
#include "resourceFileLoader.h"
#include "resourceLoader.h"
#include "resourceKey.h"
#include "resource.h"

#include "base/io/include/ioAsyncFileHandle.h"
#include "base/object/include/streamOpcodeReader.h"
#include "base/object/include/object.h"

namespace base
{
    namespace res
    {
        //--

        void ResolveStringIDs(const FileTables& tables, const FileLoadingContext& context, stream::OpcodeResolvedReferences& resolvedReferences)
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

        void ResolveTypes(const FileTables& tables, const FileLoadingContext& context, stream::OpcodeResolvedReferences& resolvedReferences)
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

        void ResolveProperties(const FileTables& tables, const FileLoadingContext& context, stream::OpcodeResolvedReferences& resolvedReferences)
        {
            const auto numProperties = tables.chunkCount(FileTables::ChunkType::Properties);
            resolvedReferences.properties.resize(numProperties);
            resolvedReferences.propertyNames.resize(numProperties);

            const auto* ptr = tables.propertyTable();
            for (uint32_t i = 0; i < numProperties; ++i, ++ptr)
            {
                auto classType = resolvedReferences.types[ptr->classTypeIndex].toClass();
                auto propertyName = resolvedReferences.stringIds[ptr->nameIndex];

                const rtti::Property* prop = nullptr;
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

        void ResolvePath(const FileTables& tables, uint32_t pathIndex, StringBuilder& str)
        {
            if (pathIndex != 0)
            {
                const auto& pathEntry = tables.pathTable()[pathIndex];

                if (pathEntry.parentIndex != 0)
                {
                    ResolvePath(tables, pathEntry.parentIndex, str);
                    str.append("/");
                }

                const auto* pathString = tables.stringTable() + pathEntry.stringIndex;
                //DEBUG_CHECK(ValidateFileNameWithExtension(pathString));
                str.append(pathString);
            }
        }

        void ResolveImports(const FileTables& tables, const FileLoadingContext& context, stream::OpcodeResolvedReferences& resolvedReferences)
        {
            const auto numImports = tables.chunkCount(FileTables::ChunkType::Imports);
            resolvedReferences.resources.resize(numImports);

            Array<uint32_t> resourcesToLoad;
            resourcesToLoad.reserve(numImports);

            const auto* ptr = tables.importTable();
            for (uint32_t i = 0; i < numImports; ++i, ++ptr)
            {
                auto classType = resolvedReferences.types[ptr->classTypeIndex].toClass();

                StringBuilder depotPath;
                depotPath.append("/"); // TODO: "is relative" flag ?
                ResolvePath(tables, ptr->pathIndex, depotPath);

                DEBUG_CHECK(ValidateDepotPath(depotPath.view(), DepotPathClass::AbsoluteFilePath));

                resolvedReferences.resources[i].type = classType;
                resolvedReferences.resources[i].path = depotPath.toString();

                // load ?
                auto flagLoad = (0 != (ptr->flags & FileTables::ImportFlag_Load));
                if (context.resourceLoader && flagLoad)
                    resourcesToLoad.pushBack(i);
            }

            // load the imports
            if (context.resourceLoader)
            {
                auto allLoadedSignal = Fibers::GetInstance().createCounter("WaitForImports", resourcesToLoad.size());

                for (auto index : resourcesToLoad)
                {
                    auto& entry = resolvedReferences.resources[index];

                    RunChildFiber("LoadImport") << [&entry, &context, &allLoadedSignal](FIBER_FUNC)
                    {
                        const auto key = ResourceKey(ResourcePath(entry.path), entry.type.cast<IResource>());
                        entry.loaded = context.resourceLoader->loadResource(key);
                        if (!entry.loaded)
                            TRACE_WARNING("Loader: Missing reference to file '{}'", key);

                        Fibers::GetInstance().signalCounter(allLoadedSignal);
                    };
                }

                Fibers::GetInstance().waitForCounterAndRelease(allLoadedSignal);
            }
        }

        void ResolveExports(const FileTables& tables, FileLoadingContext& context, stream::OpcodeResolvedReferences& resolvedReferences)
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

                if (i == 0 && context.mutatedRootClass)
                    classType = context.mutatedRootClass;

                ObjectPtr parentObject;
                if (context.loadSpecificClass)
                {
                    if (classType->is(context.loadSpecificClass))
                    {
                        parentObject = nullptr;
                    }
                    else if (ptr->parentIndex != 0)
                    {
                        parentObject = resolvedReferences.objects[ptr->parentIndex - 1];
                        if (!parentObject)
                            continue;
                    }
                    else
                    {
                        continue;
                    }
                }
                else if (ptr->parentIndex != 0)
                {
                    parentObject = resolvedReferences.objects[ptr->parentIndex - 1];
                    if (!parentObject)
                        continue;
                }

                auto obj = classType->create<IObject>();
                resolvedReferences.objects[i] = obj;

                if (context.resourceLoader && context.resourceLoadPath)
                {
                    if (i == 0 && obj->cls()->is<res::IResource>())
                    {
                        auto* resource = static_cast<res::IResource*>(obj.get());
                        resource->bindToLoader(context.resourceLoader, context.resourceLoadPath);
                    }
                }

                if (parentObject)
                    obj->parent(parentObject);
                else
                    context.loadedRoots.pushBack(obj);
            }
        }

        void ResolveReferences(const FileTables& tables, FileLoadingContext& context, stream::OpcodeResolvedReferences& resolvedReferences)
        {
            ResolveStringIDs(tables, context, resolvedReferences);
            ResolveTypes(tables, context, resolvedReferences);
            ResolveProperties(tables, context, resolvedReferences);
            ResolveImports(tables, context, resolvedReferences);
            ResolveExports(tables, context, resolvedReferences);
        }

        //--
        static const uint64_t DefaultLoadBufferSize = 8U << 20;
        static const uint64_t BlockSize = 4096;

        uint64_t DetermineLoadBufferSize(io::IAsyncFileHandle* file, const FileTables& tables, FileLoadingContext& context, const stream::OpcodeResolvedReferences& resolvedReferences)
        {
            // whole file is smaller than the load buffer
            // NOTE: use this ONLY if we indeed want to load the whole file
            if (!context.loadSpecificClass)
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

        bool LoadFileObjects(io::IAsyncFileHandle* file, const FileTables& tables, FileLoadingContext& context)
        {
            // do we have "safe layout" in the file ?
            const bool protectedFileLayout = 0 != (tables.header()->flags & FileTables::FileFlag_ProtectedLayout);

            // resolve all references
            // NOTE: this will also create all objects that we want to load
            stream::OpcodeResolvedReferences resolvedReferences;
            ResolveReferences(tables, context, resolvedReferences);

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
                        stream::OpcodeReader reader(resolvedReferences, objectData, objectDataSize, protectedFileLayout, tables.header()->version);
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

        bool LoadFileTables(io::IAsyncFileHandle* file, Buffer& tablesData)
        {
            // no file
            if (!file)
                return false;

            // load the header's worth of data
            FileTables::Header header;
            if (file->readAsync(0, sizeof(header), &header) != sizeof(header))
            {
                TRACE_WARNING("LoadFile: Failed to load header");
                return false;
            }

            // check the header
            if (!FileTables::ValidateHeader(header))
            {
                TRACE_WARNING("LoadFile: Failed to validate header");
                return false;
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

        bool LoadFile(io::IAsyncFileHandle* file, FileLoadingContext& context)
        {
            // load file tables
            Buffer tablesData;
            if (!LoadFileTables(file, tablesData))
                return false;
         
            // load full file or just few objects
            const auto& tables = *(FileTables*)tablesData.data();
            if (!LoadFileObjects(file, tables, context))
                return false;

            return true;
        }

        //--

        FileLoadingDependency::FileLoadingDependency()
        {}

        bool LoadFileDependencies(io::IAsyncFileHandle* file, const FileLoadingContext& context, Array<FileLoadingDependency>& outDependencies)
        {
            // load file tables
            Buffer tablesData;
            if (!LoadFileTables(file, tablesData))
                return false;

            // resolve table data
            const auto& tables = *(FileTables*)tablesData.data();
            stream::OpcodeResolvedReferences resolvedReferences;
            ResolveStringIDs(tables, context, resolvedReferences);
            ResolveTypes(tables, context, resolvedReferences);
            ResolveImports(tables, context, resolvedReferences);

            // list imports
            for (const auto& info : resolvedReferences.resources)
            {
                if (const auto resourceClass = info.type.cast<IResource>())
                {
                    auto& outEntry = outDependencies.emplaceBack();
                    outEntry.key = ResourceKey(ResourcePath(info.path), resourceClass);
                    outEntry.loaded = info.loaded;
                }
            }

            return true;
        }

        //--

        MetadataPtr LoadFileMetadata(io::IAsyncFileHandle* file, const FileLoadingContext& context)
        {
            FileLoadingContext loadingContext;
            loadingContext.loadSpecificClass = Metadata::GetStaticClass();

            if (!LoadFile(file, loadingContext))
                return nullptr;

            return loadingContext.root<Metadata>();
        }

        //--

    } // res
} // base