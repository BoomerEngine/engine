/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#include "build.h"
#include "fileTables.h"
#include "fileSaver.h"
#include "core/io/include/asyncFileHandle.h"
#include "core/object/include/object.h"
#include "core/object/include/streamOpcodes.h"
#include "core/object/include/streamOpcodeWriter.h"
#include "core/object/include/streamOpcodeBinarizer.h"
#include "core/containers/include/queue.h"
#include "core/io/include/fileHandle.h"
#include "fileTablesBuilder.h"

BEGIN_BOOMER_NAMESPACE()

//--

bool FileSavingContext::shouldSave(const IObject* object) const
{
    while (object)
    {
        if (rootObject.contains(object))
            return true;
        object = object->parent();
    }

    return false;
}

//--

struct FileSerializedObject : public NoCopy
{
    RTTI_DECLARE_POOL(POOL_SERIALIZATION)

public:
    FileSerializedObject* parent = nullptr;
    ObjectPtr object;
    stream::OpcodeWriterReferences localReferences;
    stream::OpcodeStream stream;
};

struct FileSavingState
{
    const Array<ObjectPtr>& roots; // only those objects and objects below can be saved
    Array<ObjectPtr> unsavedObjects; // objects yet unsaved

    //stream::OpcodeMappedReferences mappedReferencesferences; // "knowledge" repository - merged

    Array<FileSerializedObject*> serializedObjects;
    HashMap<const IObject*, FileSerializedObject*> serializedObjectMap;

    FileSavingState(const Array<ObjectPtr>& roots_)
        : roots(roots_)
    {}


};

//--

bool SaveSingleObject(FileSerializedObject& objectState)
{
    stream::OpcodeWriter writer(objectState.stream, objectState.localReferences);
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

bool CollectObjects(const FileSavingContext& context, FileSerializedObjectCollection& outCollection, IProgressTracker* progress)
{
    FileSerializedObjectQueue objectQueue;

    // start saving with the root objects
    for (const auto& obj : context.rootObject)
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
        stream::OpcodeWriter writer(obj->stream, obj->localReferences);
        obj->object->onWriteBinary(writer);
        if (obj->stream.corrupted())
        {
            TRACE_WARNING("Opcode stream corruption at object '{}' 0x{}. Possible OOM.", obj->object->cls()->name(), Hex(obj->object.get()));
            return false;
        }

        // make sure referenced objects get saved as well (if applicable)
        for (const auto& referencedObject : obj->localReferences.objects.keys())
        {
            if (context.shouldSave(referencedObject))
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

void BuildFileTables(const Array<FileSerializedObject*>& objects, FileTablesBuilder& outTables, stream::OpcodeMappedReferences& outMappedReferences)
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
            const auto index = outTables.mapImport(ref.resourceType->name(), ref.resourcePath, false);
            DEBUG_CHECK(index != 0);
            outMappedReferences.mappedResources[ref] = index;
        }
    }

    for (const auto* obj : objects)
    {
        for (const auto& ref : obj->localReferences.asyncResources.keys())
        {
            const auto index = outTables.mapImport(ref.resourceType->name(), ref.resourcePath, true);
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
}

bool WriteObjects(const FileSavingContext& context, const FileSerializedObjectCollection& objects, const stream::OpcodeMappedReferences& mappedReferences, FileTablesBuilder& tables, uint64_t baseOffset, IWriteFileHandle* file, IProgressTracker* progress)
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
            stream::OpcodeFileWriter fileWriter(file);
            stream::WriteOpcodes(context.protectedStream, object->stream, mappedReferences, fileWriter);
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

uint32_t HeaderFlags(const FileSavingContext& context)
{
    uint32_t flags = 0;

    if (context.protectedStream)
        flags |= FileTables::FileFlag_ProtectedLayout;

    return flags;
}

bool SaveFile(IWriteFileHandle* file, const FileSavingContext& context, IProgressTracker* progress)
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
    stream::OpcodeMappedReferences mappedReferences;
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

    // write buffers

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

END_BOOMER_NAMESPACE()

//--

BEGIN_BOOMER_NAMESPACE()

void ExtractUsedResources(const IObject* object, HashMap<ResourcePath, uint32_t>& outResourceCounts)
{
    if (object)
    {
        FileSavingContext context;
        context.rootObject.pushBack(AddRef(object));

        FileSerializedObjectCollection objectCollection;
        if (CollectObjects(context, objectCollection, nullptr))
        {
            FileTablesBuilder fileTables;
            stream::OpcodeMappedReferences mappedReferences;
            BuildFileTables(objectCollection.orderedObjects(), fileTables, mappedReferences);

            for (const auto& resourceRef : mappedReferences.mappedResources.keys())
            {
                const auto key = ResourcePath(resourceRef.resourcePath);
                outResourceCounts[key] += 1;
            }
        }
    }
}

END_BOOMER_NAMESPACE()
