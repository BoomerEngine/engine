/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#include "build.h"
#include "fileTables.h"
#include "fileTablesBuilder.h"
#include "fileSaver.h"

#include "core/io/include/asyncFileHandle.h"
#include "core/io/include/fileHandle.h"
#include "core/object/include/object.h"
#include "core/object/include/serializationStream.h"
#include "core/object/include/serializationWriter.h"
#include "core/object/include/serializationBinarizer.h"
#include "core/object/include/asyncBuffer.h"
#include "core/containers/include/queue.h"

BEGIN_BOOMER_NAMESPACE()

#pragma optimize("",off)

//--

struct FileSerializedObject : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_SERIALIZATION)

public:
    FileSerializedObject* parent = nullptr;
    ObjectPtr object;
    SerializationWriterReferences localReferences;
    SerializationStream stream;
};

struct FileSavingState
{
    const Array<ObjectPtr>& roots; // only those objects and objects below can be saved
    Array<ObjectPtr> unsavedObjects; // objects yet unsaved

    //SerializationMappedReferences mappedReferencesferences; // "knowledge" repository - merged

    Array<FileSerializedObject*> serializedObjects;
    HashMap<const IObject*, FileSerializedObject*> serializedObjectMap;

    FileSavingState(const Array<ObjectPtr>& roots_)
        : roots(roots_)
    {}
};

//--

bool SaveSingleObject(FileSerializedObject& objectState)
{
    SerializationWriter writer(objectState.stream, objectState.localReferences);
    objectState.object->onWriteBinary(writer);
    return !objectState.stream.corrupted();
}

//--

class FileSerializedObjectCollection : public NoCopy
{
public:
    ~FileSerializedObjectCollection()
    {
        m_objectMap.clearPtr();
    }

    // gets final list of objects to saved, parents are before children
    INLINE const Array< FileSerializedObject*>& orderedObjects() const
    {
        return m_orderedObjects;
    }

    FileSerializedObject* mapObject(const IObject* object)
    {
        if (!object)
            return nullptr;

        FileSerializedObject* ret = nullptr;
        if (!m_objectMap.find(object, ret))
        {
            ret = new FileSerializedObject;
            ret->object = AddRef(object);
            ret->parent = nullptr;
            m_objectMap[object] = ret;
        }

        return ret;
    }

    void orderObjects()
    {
        InplaceArray<const IObject*, 20> objectParents;
        HashSet<const IObject*> visitedObjects;

        visitedObjects.reserve(m_objectMap.size() * 2);
        m_orderedObjects.reserve(m_objectMap.size());

        for (FileSerializedObject* obj : m_objectMap.values())
        {
            // extract real object parents
            objectParents.reset();
            for (const auto* ptr = obj->object.get(); ptr; ptr = ptr->parent())
                objectParents.pushBack(ptr);

            // make sure parents are before children
            for (int i = objectParents.lastValidIndex(); i >= 0; --i)
            {
                const auto* ptr = objectParents[i];
                if (visitedObjects.insert(ptr))
                {
                    FileSerializedObject* ret = nullptr;
                    if (m_objectMap.find(ptr, ret))
                    {
                        ASSERT(ret != nullptr);
                        m_orderedObjects.pushBack(ret);
                    }
                }
            }
        }
    }

private:
    HashMap<const IObject*, FileSerializedObject*> m_objectMap;
    Array<FileSerializedObject*> m_orderedObjects;
};

//---

class FileSerializedObjectQueue : public NoCopy
{
public:
    FileSerializedObjectQueue()
    {}

    void push(FileSerializedObject* object)
    {
        if (object && m_visitedObjects.insert(object))
            m_objectsToSave.push(object);
    }
                
    FileSerializedObject* pop()
    {
        if (m_objectsToSave.empty())
            return nullptr;

        auto* ret = m_objectsToSave.top();
        m_objectsToSave.pop();
        return ret;
    }

private:
    Queue<FileSerializedObject*> m_objectsToSave;
    HashSet<const FileSerializedObject*> m_visitedObjects;
};

//--

static bool ShouldSaveObject(const FileSavingContext& context, const IObject* object)
{
    while (object)
    {
        if (context.rootObjects.contains(object))
            return true;
        object = object->parent();
    }

    return false;
}

static bool CollectObjects(const FileSavingContext& context, FileSerializedObjectCollection& outCollection, IProgressTracker* progress)
{
    FileSerializedObjectQueue objectQueue;

    // start saving with the root objects
    for (const auto* obj : context.rootObjects)
        objectQueue.push(outCollection.mapObject(obj));

    // save objects
    uint32_t numSavedObjects = 0;
    while (auto* obj = objectQueue.pop())
    {
        // support cancellation - files can get pretty big...
        if (progress && progress->checkCancelation())
            return false;

        // update progress
        if (progress)
            progress->reportProgress(numSavedObjects, 0, TempString("Serializing object {}: {}", numSavedObjects, obj->object));

        // serialize object to opcodes
        SerializationWriter writer(obj->stream, obj->localReferences);
        obj->object->onWriteBinary(writer);
        if (obj->stream.corrupted())
        {
            TRACE_WARNING("Opcode stream corruption at object '{}' 0x{}. Possible OOM.", obj->object->cls()->name(), Hex(obj->object.get()));
            return false;
        }

        // make sure referenced objects get saved as well (if applicable)
        for (const auto& referencedObject : obj->localReferences.objects.keys())
        {
            if (ShouldSaveObject(context, referencedObject))
            {
                objectQueue.push(outCollection.mapObject(referencedObject));
            }
            else
            {
                TRACE_INFO("Lost pointer to '{}' from '{}' because object is outside the save hierarchy", referencedObject, obj->object);
            }
        }

        numSavedObjects += 1;
    }

    // order object so parents come before children
    outCollection.orderObjects();
    return true;
}

static void BuildFileTables(const Array<FileSerializedObject*>& objects, FileTablesBuilder& outTables, SerializationMappedReferences& outMappedReferences)
{
    // NOTE: maintain determinism

    for (const auto* obj : objects)
    {
        for (const auto name : obj->localReferences.stringIds.keys())
        {
            const auto index = outTables.mapName(name);
            //TRACE_INFO("Name '{}' mapped to {}", name, index);
            outMappedReferences.mappedNames[name] = index;
        }
    }

    for (const auto* obj : objects)
    {
        for (const auto type : obj->localReferences.types.keys())
        {
            const auto index = outTables.mapType(type);
            //TRACE_INFO("Type '{}' mapped to {}", type, index);
            outMappedReferences.mappedTypes[type] = index;
        }
    }

    for (const auto* obj : objects)
    {
        for (const auto* prop : obj->localReferences.properties.keys())
        {
            const auto index = outTables.mapProperty(prop);
            outMappedReferences.mappedProperties[prop] = index;
        }
    }

    for (const auto* obj : objects)
    {
        for (const auto& ref : obj->localReferences.syncResources.keys())
        {
            const auto index = outTables.mapImport(ref.resourceType->name(), ref.resourceID, false);
            DEBUG_CHECK(index != 0);
            outMappedReferences.mappedResources[ref] = index;
        }
    }

    for (const auto* obj : objects)
    {
        for (const auto& ref : obj->localReferences.asyncResources.keys())
        {
            const auto index = outTables.mapImport(ref.resourceType->name(), ref.resourceID, true);
            DEBUG_CHECK(index != 0);
            outMappedReferences.mappedResources[ref] = index;
        }
    }

    // unless object gets exported map it as NULL
    for (const auto* obj : objects)
    {
        for (const auto& ref : obj->localReferences.objects.keys())
        {
            outMappedReferences.mappedPointers[ref.get()] = 0;
        }
    }

    // gather all referenced async buffers
    for (const auto* obj : objects)
    {
        for (const auto& ref : obj->localReferences.asyncBuffers.keys())
        {
            outMappedReferences.mappedAsyncBuffers.insert(ref);
        }
    }

    // build object table and export objects
    for (uint32_t i=0; i<objects.size(); ++i)
    {
        const auto* obj = objects[i];

        auto& exportInfo = outTables.exportTable.emplaceBack();
        exportInfo.classTypeIndex = outTables.mapType(obj->object->cls());
        //TRACE_INFO("Object type '{}' mapped to {}", obj->object->cls(), exportInfo.classTypeIndex);

        uint32_t parentIndex = 0;
        if (outMappedReferences.mappedPointers.find(obj->object->parent(), parentIndex))
            exportInfo.parentIndex = parentIndex;
        else
            exportInfo.parentIndex = 0;

        //TRACE_INFO("Mapped object {}/{}: {}", i + 1, objects.size(), obj->object);
        outMappedReferences.mappedPointers[obj->object.get()] = i + 1;
    }

    // sort buffers by the CRC to allow for interpolation search
    auto bufferKeys = outMappedReferences.mappedAsyncBuffers.keys();
    std::sort(bufferKeys.begin(), bufferKeys.end(), [](const auto& a, const auto& b) { return a->crc() < b->crc(); });

    // build buffer table
    for (const auto& buffer : bufferKeys)
    {
        uint64_t crc = buffer->crc();
        if (!outTables.bufferRawMap.contains(crc))
        {
            FileTablesBuilder::BufferData data;
            if (buffer->extract(data.compressedData, data.compressionType) && data.compressedData)
            {
                data.crc = crc;
                data.uncompressedSize = buffer->size();
                outTables.bufferData.pushBack(data);

                auto& entry = outTables.bufferTable.emplaceBack();
                entry.crc = crc;
                entry.compressionType = (uint8_t)data.compressionType;
                entry.compressedSize = data.compressedData.size();
                entry.uncompressedSize = buffer->size();
                entry.dataOffset = 0; // not yet saved
            }
        }
    }
}

static bool WriteObjects(const FileSavingContext& context, const FileSerializedObjectCollection& objects, const SerializationMappedReferences& mappedReferences, FileTablesBuilder& tables, uint64_t baseOffset, IWriteFileHandle* file, IProgressTracker* progress)
{
    const auto numObjects = objects.orderedObjects().size();
    for (uint32_t i = 0; i < numObjects; ++i)
    {
        const auto* object = objects.orderedObjects().typedData()[i];

        // support cancellation of saving
        if (progress && progress->checkCancelation())
            return false;

        // update saving progress
        if (progress)
            progress->reportProgress(i + 1, numObjects, TempString("Saving object {}: {}", i, object->object));

        // write binary opcode stream to file
        const auto objectStartPos = file->pos();
        {
            SerializationBinaryPacker fileWriter(file);
            WriteOpcodes(context.format == FileSaveFormat::ProtectedBinaryStream, object->stream, mappedReferences, fileWriter);
            fileWriter.flush();

            tables.exportTable[i].crc = fileWriter.crc();
        }

        // patch object entry
        tables.exportTable[i].dataOffset = objectStartPos - baseOffset;
        tables.exportTable[i].dataSize = file->pos() - objectStartPos;
    }

    // all objects saved
    return true;
}

static uint32_t HeaderFlags(const FileSavingContext& context)
{
    uint32_t flags = 0;

    if (context.format == FileSaveFormat::ProtectedBinaryStream)
        flags |= FileTables::FileFlag_ProtectedLayout;

    if (context.extractBuffers)
        flags |= FileTables::FileFlag_ExtractedBuffers;

    return flags;
}

static bool SaveFileBinary(IWriteFileHandle* file, const FileSavingContext& context, FileSavingResult& outResult, IProgressTracker* progress)
{
    ScopeTimer timer;

    // collect objects to save
    FileSerializedObjectCollection objectCollection;
    if (!CollectObjects(context, objectCollection, progress))
    {
        file->discardContent();
        return false;
    }

    // merge reference tables
    FileTablesBuilder fileTables;
    SerializationMappedReferences mappedReferences;
    BuildFileTables(objectCollection.orderedObjects(), fileTables, mappedReferences);

    // store the file header to reserve space in the final file
    // NOTE: header size will not change since we gathered and mapped all references
    const auto baseOffset = file->pos();
    if (!fileTables.write(file, HeaderFlags(context), 0, 0))
    {
        file->discardContent();
        return false;
    }

    // write objects
    if (!WriteObjects(context, objectCollection, mappedReferences, fileTables, baseOffset, file, progress))
    {
        file->discardContent();
        return false;
    }

    // remember file position after all objects were written
    const auto objectEnd = file->pos() - baseOffset;

    // write or extract buffers
    ASSERT(fileTables.bufferData.size() == fileTables.bufferTable.size());
    for (auto i : fileTables.bufferData.indexRange())
    {
        const auto& data = fileTables.bufferData[i];

        if (progress->checkCancelation())
        {
            file->discardContent();
            return false;
        }

        if (context.extractBuffers)
        {
            auto& entry = outResult.extractedBuffers.emplaceBack();
            entry.compressedData = data.compressedData;
            entry.compressionType = data.compressionType;
            entry.uncompressedSize = data.uncompressedSize;
            entry.uncompressedCRC = data.crc;
        }
        else
        {
            auto& entry = fileTables.bufferTable[i];
            entry.dataOffset = file->pos() - baseOffset;
            entry.compressedSize = data.compressedData.size();

            const auto numWritten = data.compressedData.size();
            if (file->writeSync(data.compressedData.data(), data.compressedData.size()) != numWritten)
            {
                TRACE_WARNING("Failed to save {} bytes of async buffer {}, saved {} only", data.compressedData.size(), data.crc, numWritten);
                file->discardContent();
                return false;
            }
        }
    }   

    // remember file position after all buffers were written
    const auto buffersEnd = file->pos() - baseOffset;
    const auto bufferDataSize = buffersEnd - objectEnd;

    // write the final header again
    file->pos(baseOffset);
    if (!fileTables.write(file, HeaderFlags(context), objectEnd, buffersEnd))
    {
        file->pos(buffersEnd);
        file->discardContent();
        return false;
    }

    // go back
    file->pos(buffersEnd);

    // done
    TRACE_INFO("Saved {} objects ({} of objects, {} of buffers) in {}",
        objectCollection.orderedObjects().size(), MemSize(objectEnd), MemSize(bufferDataSize), timer);
    return true;
}

//--

static bool SaveFileText(IWriteFileHandle* file, const FileSavingContext& context, FileSavingResult& outResult, IProgressTracker* progress)
{
    return false;
}

//--

bool SaveFile(IWriteFileHandle* file, const FileSavingContext& context, FileSavingResult& outResult, IProgressTracker* progress)
{
    if (context.format == FileSaveFormat::XML)
        return SaveFileText(file, context, outResult, progress);
    else
        return SaveFileBinary(file, context, outResult, progress);
}

END_BOOMER_NAMESPACE()

//--

BEGIN_BOOMER_NAMESPACE()

void ExtractUsedResources(const IObject* object, HashMap<ResourceID, uint32_t>& outResourceCounts)
{
    if (object)
    {
        FileSavingContext context;
        context.rootObjects.pushBack(object);

        FileSerializedObjectCollection objectCollection;
        if (CollectObjects(context, objectCollection, nullptr))
        {
            FileTablesBuilder fileTables;
            SerializationMappedReferences mappedReferences;
            BuildFileTables(objectCollection.orderedObjects(), fileTables, mappedReferences);

            for (const auto& resourceRef : mappedReferences.mappedResources.keys())
                outResourceCounts[resourceRef.resourceID] += 1;
        }
    }
}

END_BOOMER_NAMESPACE()
