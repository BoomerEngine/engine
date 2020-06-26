/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization\binary #]
***/

#include "build.h"

#include "resource.h"
#include "resourceBinaryFileTables.h"
#include "resourceBinaryRuntimeTables.h"
#include "resourceLoader.h"

#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/streamBinaryVersion.h"
#include "base/object/include/serializationLoader.h"

extern std::atomic<int> GNumWaitingImportTables;

namespace base
{
    mem::PoolID POOL_BUFFER_DATA("Engine.Buffer");

    namespace res
    {
        namespace binary
        {

            RuntimeTables::RuntimeTables()
            {
            }

            void RuntimeTables::resolve(const stream::LoadingContext& context, const FileTables& fileTables)
            {
                PC_SCOPE_LVL1(ResolveBinaryTables);

                // Create runtime data
                m_mappedNames.resize(fileTables.m_names.size());
                m_mappedTypes.resize(fileTables.m_names.size());
                m_mappedProperties.resize(fileTables.m_properties.size());
                m_mappedImports.resize(fileTables.m_imports.size());
                m_mappedExports.resize(fileTables.m_exports.size());
                m_mappedBuffers.resize(fileTables.m_buffers.size());

                // Resolve data
                resolveNames(context, fileTables);
                resolveImports(context, fileTables);
                resolveProperties(context, fileTables);
                resolveExports(context, fileTables);
                resolveBuffers(context, fileTables);
            }

            void RuntimeTables::createExports(const stream::LoadingContext& context, const FileTables& fileTables)
            {
                PC_SCOPE_LVL1(CreateObjects);

                bool selectiveLoadingObjectSelected = false;
                for (uint32_t exportIndex = 0; exportIndex < m_mappedExports.size(); ++exportIndex)
                {
                    auto& ex = fileTables.m_exports[exportIndex];
                    auto& runtime = m_mappedExports[exportIndex];

                    // skip the object
                    if (runtime.skip)
                        continue;

                    // Determine parent object
                    // NOTE: in the selective loading mode the loaded object is never parented
                    ObjectPtr objectParent;
                    if (ex.m_parent != 0 && context.m_selectiveLoadingClass == nullptr)
                    {
                        auto& parent = m_mappedExports[ex.m_parent - 1];
                        if (parent.skip)
                        {
                            runtime.skip = true; // we want to skip the whole object chain
                            continue;
                        }

                        // get the resolved object
                        objectParent = parent.object;
                    }

                    // Determine object class
                    auto objectClass = runtime.objectClass;
                    if (!objectClass)
                    {
                        TRACE_ERROR("Unknown object class '{}', object will not be loaded", runtime.className);
                        runtime.skip = true;
                        continue;
                    }

                    // Mutate object
                    if (ex.m_parent == 0 && context.m_rootObjectMutatedClass)
                    {
                        objectClass = context.m_rootObjectMutatedClass.cast<IObject>();
                    }

                    // Don't allow abstract classes
                    if (objectClass->isAbstract())
                    {
                        TRACE_ERROR("Object class '{}' is abstract, object will not be loaded", runtime.className);
                        runtime.skip = true;
                        continue;
                    }

                    // We can only load object classes
                    if (!objectClass->is<IObject>())
                    {
                        TRACE_ERROR("Object class '{}' is not derived from IObject, object will not be loaded", runtime.className);
                        runtime.skip = true;
                        continue;
                    }

                    // Object cannot be already created
                    DEBUG_CHECK_EX(!runtime.object, "Object already created");

                    // In selective loading we load only allowed classes
                    if (context.m_selectiveLoadingClass != nullptr)
                    {
                        if (selectiveLoadingObjectSelected)
                        {
                            // we must skip this object as the selective loading object was already selected
                            runtime.skip = true;
                            continue;
                        }
                        else if (objectClass->is(context.m_selectiveLoadingClass))
                        {
                            // this object will be loaded
                            selectiveLoadingObjectSelected = true;
                        }
                        else
                        {
                            // this is not our object to load
                            runtime.skip = true;
                            continue;
                        }
                    }

                    // Create new empty export
                    ObjectPtr createdObject = objectClass->create<IObject>();
                    DEBUG_CHECK_EX(createdObject, "Unable to create object even though all checks passed");

                    // link with parent object
                    createdObject->parent(objectParent);
                    runtime.object = createdObject;
                }
            }

            bool RuntimeTables::loadExports(uint64_t baseOffset, stream::IBinaryReader& file, const stream::LoadingContext& context, const FileTables& fileTables, stream::LoadingResult& outResult)
            {
                PC_SCOPE_LVL1(LoadObjects);
                bool status = true;

                for (uint32_t exportIndex = 0; exportIndex < m_mappedExports.size(); ++exportIndex)
                {
                    auto& ex = fileTables.m_exports[exportIndex];
                    auto& runtime = m_mappedExports[exportIndex];

                    // No data, skip the object
                    if (runtime.skip || !runtime.object)
                        continue;

                    // Move to file position
                    uint64_t objectStart = baseOffset + ex.m_dataOffset;
                    file.seek(objectStart);
                    file.clearErorr();

                    // Load object
                    file.m_unmapper = this;
                    bool loaded = runtime.object->onReadBinary(file);
                    file.m_unmapper = nullptr;

                    // Emit exported root
                    if (ex.m_parent == 0 || (context.m_selectiveLoadingClass != nullptr))
                        outResult.m_loadedRootObjects.pushBack(runtime.object);

#ifdef BUILD_RELEASE
                    // exit right away
                    if (!loaded || file.isError())
                        return false;
#else
                    // merge loading status - this is so we can report as much errors as possible
                    status &= (loaded && !file.isError());

                    // Seek past the end of the file ?
                    auto objectEnd = baseOffset + ex.m_dataOffset + ex.m_dataSize;
                    if (file.isError())
                    {
                        TRACE_ERROR("Export {} ('{}') caused file IO errors when reading (start={}, size={}): {}",
                                    exportIndex, runtime.objectClass->name().c_str(), objectStart, ex.m_dataSize, file.lastError().c_str());
                    }
                    else if (file.pos() > objectEnd)
                    {
                        TRACE_ERROR("Export {} ('{}') accessed data beyond it's location in file (offset={}, start={}, stop={})",
                                    exportIndex, runtime.objectClass->name().c_str(), file.pos(), objectStart, objectEnd);
                    }
#endif
                }

                return status;
            }

            void RuntimeTables::postLoad()
            {
                PC_SCOPE_LVL1(PostLoad);
                for (auto& runtime : m_mappedExports)
                {
                    if (runtime.object)
                        runtime.object->onPostLoad();
                }
            }

            void RuntimeTables::loadBuffers(stream::IBinaryReader& file, const FileTables& fileTables)
            {
                PC_SCOPE_LVL1(LoadBuffers);
                for (uint32_t i=0; i<m_mappedBuffers.size(); ++i)
                {
                    auto& buffer = m_mappedBuffers[i];
                    auto& tableBuffer = fileTables.m_buffers[i];

                    // if we don't have the access load content directly
                    if (buffer.access == nullptr)
                    {
                        // load disk content (compressed or not)
                        auto diskContent = Buffer::Create(POOL_BUFFER_DATA, tableBuffer.m_dataSizeOnDisk);
                        if (!diskContent)
                        {
                            TRACE_WARNING("Failed to allocate load buffer for buffer {} ({} bytes)", i, tableBuffer.m_dataSizeOnDisk);
                            continue;
                        }

                        // load crap
                        file.seek(tableBuffer.m_dataOffset);
                        file.read((void*)diskContent.data(), diskContent.size());
                        if (file.isError())
                        {
                            TRACE_WARNING("Failed to disk data for buffer {} ({} bytes): {}", i, tableBuffer.m_dataSizeOnDisk, file.lastError());
                            file.clearErorr();
                            continue;
                        }

                        // decompress
                        auto memoryContent = diskContent;
                        if (tableBuffer.m_dataSizeInMemory != tableBuffer.m_dataSizeOnDisk)
                        {
                            auto decompressedData = Buffer::Create(POOL_BUFFER_DATA, tableBuffer.m_dataSizeInMemory);
                            if (!decompressedData)
                            {
                                TRACE_WARNING("Failed to allocate load buffer for decompressed buffer {} ({} bytes)", i, tableBuffer.m_dataSizeInMemory);
                                continue;
                            }

                            if (!mem::Decompress(mem::CompressionType::LZ4HC, diskContent.data(), diskContent.size(), decompressedData.data(), decompressedData.size()))
                            {
                                TRACE_WARNING("Failed to decompress data for buffer {} ({}->{} bytes)", i, tableBuffer.m_dataSizeOnDisk, tableBuffer.m_dataSizeInMemory);
                                continue;
                            }

                            // validate data integrity
#if !defined(BUILD_RELEASE)
                            auto actualDataCRC = CRC64().append(memoryContent.data(), memoryContent.size()).crc();
                            if (actualDataCRC != tableBuffer.m_crc)
                            {
                                TRACE_WARNING("Buffer {} data corruption detected", i);
                                continue;
                            }
#endif
                        }

                        // create the persistent access to buffer
                        buffer.access = CreateSharedPtr<stream::PreloadedBufferLatentLoader>(memoryContent, tableBuffer.m_crc);
                    }
                }
            }

            //----

            void RuntimeTables::unmapName(const stream::MappedNameIndex index, StringID& outName)
            {
                if (index == 0)
                {
                    outName = StringID();
                    return;
                }

                auto arrayIndex = index - 1U;
                if (arrayIndex >= m_mappedNames.size())
                {
                    TRACE_ERROR("Invalid name index {} (of {}). Mapping to None.", arrayIndex, m_mappedNames.size());
                    outName = StringID();
                    return;
                }

                outName = m_mappedNames[arrayIndex];
            }

            void RuntimeTables::unmapType(const stream::MappedTypeIndex index, Type& outTypeRef)
            {
                if (index == 0)
                {
                    outTypeRef = nullptr;
                    return;
                }

                auto arrayIndex = index - 1U;
                if (arrayIndex >= m_mappedNames.size())
                {
                    TRACE_ERROR("Invalid type ref index {} (of {}). Mapping to NULL.", arrayIndex, m_mappedNames.size());
                    outTypeRef = nullptr;
                    return;
                }

                if (!m_mappedTypes[arrayIndex])
                {
                    auto typeName = m_mappedNames[arrayIndex];
                    m_mappedTypes[arrayIndex] = RTTI::GetInstance().findType(typeName);

                    if (!m_mappedTypes[arrayIndex])
                    {
                        TRACE_ERROR("Missing referenced type '{}'", typeName);
                    }
                }

                outTypeRef = m_mappedTypes[arrayIndex];
            }

            void RuntimeTables::unmapProperty(stream::MappedPropertyIndex index, const rtti::Property*& outPropertyRef, StringID& outClassName, StringID& outPropName, StringID& outPropTypeName, Type& outType)
            {
                if (index == 0)
                {
                    outPropertyRef = nullptr;
                    return;
                }

                auto arrayIndex = index - 1U;
                if (arrayIndex >= m_mappedProperties.size())
                {
                    TRACE_ERROR("Invalid property index {} (of {}). Mapping to None.", arrayIndex, m_mappedProperties.size());
                    outPropertyRef = nullptr;
                    return;
                }

                auto& info = m_mappedProperties[arrayIndex];
                outPropertyRef = info.property;
                outClassName = info.className;
                outPropName = info.name;
                outPropTypeName = info.typeName;
                outType = info.type;
            }

            void RuntimeTables::unmapPointer(const stream::MappedObjectIndex index, ObjectPtr& outObjectRef)
            {
                if (index == 0)
                {
                    outObjectRef = nullptr;
                    return;
                }

                auto arrayIndex = index - 1U;
                if (arrayIndex >= m_mappedExports.size())
                {
                    TRACE_ERROR("Invalid export index {} (of {}). Mapping to NULL.", arrayIndex, m_mappedExports.size());
                    outObjectRef = nullptr;
                }

                auto& data = m_mappedExports[arrayIndex];
                outObjectRef = data.object;
            }
            
            void RuntimeTables::unmapResourceReference(stream::MappedPathIndex index, StringBuf& outResourcePath, ClassType& outResourceClass, ObjectPtr& outObjectPtr)
            {
                if (index == 0)
                {
                    outResourcePath = StringBuf::EMPTY();
                    outObjectPtr = nullptr;
                    return;
                }

                auto arrayIndex = index - 1U;
                if (arrayIndex >= m_mappedImports.size())
                {
                    TRACE_ERROR("Invalid resource reference index {} (of {}). Mapping to None.", arrayIndex, m_mappedImports.size());
                    outResourcePath = StringBuf::EMPTY();
                    outObjectPtr = nullptr;
                    return;
                }

                auto& info = m_mappedImports[arrayIndex];
                outResourcePath = info.path;
                outResourceClass = info.loadClass;
                outObjectPtr = info.resource;
            }

            void RuntimeTables::unmapBuffer(const stream::MappedBufferIndex index, stream::DataBufferLatentLoaderPtr& outBufferAccess)
            {
                // clear for now
                outBufferAccess.reset();

                // the index 0 is mapped to empty buffer
                if (index == 0)
                    return;

                // get the buffer entry
                auto bufferIndex = index - 1U;
                if (bufferIndex >= m_mappedBuffers.size())
                {
                    TRACE_ERROR("Invalid buffer reference index {} (of {}). Mapping to None.", bufferIndex, m_mappedBuffers.size());
                    return;
                }

                // get the buffer entry
                outBufferAccess = m_mappedBuffers[bufferIndex].access;
            }

            //---

            void RuntimeTables::resolveNames(const stream::LoadingContext& context, const FileTables& fileTables)
            {
                for (uint32_t i = 0; i < m_mappedNames.size(); ++i)
                {
                    auto str  = &fileTables.m_strings[fileTables.m_names[i].m_string];
                    m_mappedNames[i] = StringID(str);
                    m_mappedTypes[i] = nullptr; // types are resolved on request but any name can be a type name
                }
            }

            void RuntimeTables::resolveImports(const stream::LoadingContext& context, const FileTables& fileTables)
            {
                PC_SCOPE_LVL1(ResolveImports);

                InplaceArray<uint32_t, 64> resourcesToLoad;

                for (uint32_t i = 0; i<m_mappedImports.size(); ++i)
                {
                    auto& data = fileTables.m_imports[i];

                    // resolve path
                    auto string  = &fileTables.m_strings[data.m_path];
                    m_mappedImports[i].path = StringBuf(string);

                    auto className  = &fileTables.m_strings[fileTables.m_names[data.m_className - 1].m_string];
                    m_mappedImports[i].loadClass = RTTI::GetInstance().findClass(StringID(className)).cast<IResource>();
                    if (!m_mappedImports[i].loadClass)
                    {
                        TRACE_WARNING("Loaded a reference to '{}' with class '{}' that is now missing, reference will not be loaded", m_mappedImports[i].path, className);
                    }

                    // start loading (if not a soft imports)
                    if (context.m_resourceLoader != nullptr)
                    {
                        if (data.m_flags & (uint16_t)FileTables::ImportFlags::Load)
                        {
                            ResourcePath resourcePath(m_mappedImports[i].path);
                            ResourceKey key(resourcePath, m_mappedImports[i].loadClass);

                            if (auto existingResource = context.m_resourceLoader->acquireLoadedResource(key))
                            {
                                m_mappedImports[i].resource = existingResource;
                            }
                            else
                            {
                                resourcesToLoad.pushBack(i);
                            }
                        }
                    }
                }

                if (!resourcesToLoad.empty())
                {
                    PC_SCOPE_LVL1(LoadImports);

                    // create a signal to indicate end of loading
                    auto signal = Fibers::GetInstance().createCounter("WaitForImports", resourcesToLoad.size());

                    // start a fiber job to load each dependency
                    for (auto i : resourcesToLoad)
                    {
                        auto entry  = &m_mappedImports[i];
                        auto loader  = context.m_resourceLoader;

                        RunChildFiber("LoadImport") << [entry, loader, signal](FIBER_FUNC)
                        {
                            ResourceKey key(ResourcePath(entry->path), entry->loadClass);
                            entry->resource = loader->loadResource(key);
                            Fibers::GetInstance().signalCounter(signal);
                        };
                    }

                    // wait
                    {
                        PC_SCOPE_LVL1(WaitForImports);
                        Fibers::GetInstance().waitForCounterAndRelease(signal);
                    }
                }
            }

            void RuntimeTables::resolveProperties(const stream::LoadingContext& context, const FileTables& fileTables)
            {
                for (uint32_t i=0; i < m_mappedProperties.size(); ++i)
                {
                    auto& data = fileTables.m_properties[i];
                    auto& mapped = m_mappedProperties[i];

                    DEBUG_CHECK_EX(data.m_hash != 0, "Invalid property hash");

                    // We need to do a slow search
                    unmapName(data.m_className, mapped.className);
                    unmapName(data.m_propertyName, mapped.name);
                    unmapName(data.m_typeName, mapped.typeName);
                    unmapType(data.m_typeName, mapped.type);

                    // lookup in the RTTI cache
                    mapped.property = RTTI::GetInstance().findProperty(data.m_hash);

                    // repair
                    if (!mapped.property)
                    {
                        // manual search for class
                        auto classType = RTTI::GetInstance().findClass(mapped.className);
                        if (classType)
                        {
                            // search for the property in the class
                            mapped.property = classType->findProperty(mapped.name);
                            if (!mapped.property)
                            {
                                TRACE_WARNING("Missing property '{}' in class '{}'", mapped.name, mapped.className);
                            }
                        }
                    }
                }
            }

            void RuntimeTables::resolveExports(const stream::LoadingContext& context, const FileTables& fileTables)
            {
                for (uint32_t i = 0; i < m_mappedExports.size(); ++i)
                {
                    auto& data = fileTables.m_exports[i];
                    auto& mapped = m_mappedExports[i];

                    // get type name
                    unmapName(data.m_className, mapped.className);

                    // map export type
                    Type type = nullptr;
                    unmapType(data.m_className, type);

                    mapped.objectClass = type.toSpecificClass<IObject>();
                }
            }

            void RuntimeTables::resolveBuffers(const stream::LoadingContext& context, const FileTables& fileTables)
            {
                for (uint32_t i = 0; i < m_mappedBuffers.size(); ++i)
                {
                    auto& data = fileTables.m_buffers[i];
                    auto& buffer = m_mappedBuffers[i];

                    // if we have the async source specified we may try to handle the async buffers properly
                    /*if (context.m_asyncFileAccess)
                    {
                        // create a proper async access point
                        buffer.access = IBufferAsyncProxy::CreateFromResourceFile(context.m_asyncFileAccess, data.crc, data.m_dataSizeInMemory, data.m_dataSizeOnDisk);
                        if (buffer.access == nullptr)
                        {
                            TRACE_WARNING("Unable to create async buffer {} access point for file '{}'. Buffer will be loaded into memory.",
                                i, context.m_asyncFileAccess->description().c_str());
                        }
                    }*/
                }               
            }

            //---

        } // binary
    } // res
} // base