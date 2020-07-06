/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\utils #]
***/

#include "build.h"
#include "base/object/include/streamBinaryWriter.h"
#include "base/object/include/streamBinaryReader.h"
#include "base/object/include/serializationSaver.h"
#include "base/object/include/serializationLoader.h"
#include "base/object/include/serializationMapper.h"
#include "base/object/include/nativeFileReader.h"
#include "base/object/include/memoryWriter.h"
#include "base/object/include/memoryReader.h"
#include "base/object/include/nullWriter.h"

#include "base/io/include/ioSystem.h"
#include "base/io/include/ioFileHandle.h"
#include "base/io/include/absolutePath.h"
#include "base/io/include/utils.h"

#include "resource.h"
#include "resourceSerializationMetadata.h"
#include "resourceUncached.h"
#include "resourceBinarySaver.h"
#include "resourceBinaryLoader.h"
#include "resourceGeneralTextLoader.h"
#include "resourceGeneralTextSaver.h"
#include "resourcePath.h"

namespace base
{
    namespace res
    {

        ResourceHandle LoadUncached(const io::AbsolutePath& filePath, IResourceLoader* dependencyLoader, const ObjectPtr parent)
        {
            // get file extension
            auto fileExtension = filePath.fileMainExtension();

            // find resource for given extension
            auto resourceClass = IResource::FindResourceClassByExtension(TempString("{}", fileExtension));
            if (!resourceClass)
            {
                TRACE_ERROR("Unable to find resource class for '{}' (unrecognized extension '{}')", filePath, fileExtension);
                return nullptr;
            }

            return LoadUncached(filePath, resourceClass, dependencyLoader, parent);
        }

        ResourceHandle LoadUncached(const io::AbsolutePath& filePath, ClassType resourceClass, IResourceLoader* dependencyLoader, const ObjectPtr parent)
        {
            // find resource for given extension
            if (!resourceClass)
            {
                auto fileExtension = filePath.fileMainExtension();
                resourceClass = IResource::FindResourceClassByExtension(TempString("{}", fileExtension));
                if (!resourceClass)
                {
                    TRACE_ERROR("Unable to find resource class for '{}' (unrecognized extension '{}')", filePath, fileExtension);
                    return nullptr;
                }
            }

            // open file for reading
            auto fileReader = IO::GetInstance().openForReading(filePath);
            if (!fileReader)
            {
                TRACE_ERROR("Unable to open file '{}' for reading", filePath);
                return nullptr;
            }

            // load file
            stream::NativeFileReader fileStream(*fileReader);
            return LoadUncached(filePath.ansi_str().c_str(), resourceClass, fileStream, dependencyLoader, parent);
        }

        ResourceHandle LoadUncached(StringView<char> contextName, ClassType resourceClass, const void* data, uint64_t dataSize, IResourceLoader* dependencyLoader /*= nullptr*/, ObjectPtr parent /*= nullptr*/, const ResourceMountPoint& mountPoint /*= ResourceMountPoint()*/)
        {
            if (!data || !dataSize)
                return nullptr;

            stream::MemoryReader reader(data, dataSize);
            return LoadUncached(contextName, resourceClass, reader, dependencyLoader, parent, mountPoint);
        }

        ResourceHandle LoadUncached(StringView<char> contextName, ClassType resourceClass, stream::IBinaryReader& fileStream, IResourceLoader* dependencyLoader, const ObjectPtr parent, const ResourceMountPoint& mountPoint)
        {
            // create resource loader
#ifdef BUILD_FINAL
            auto loader = IResource::GetStaticClass()->findMetadataRef<SerializationLoaderMetadata>().createLoader();
#else
            auto loader = resourceClass->findMetadataRef<SerializationLoaderMetadata>().createLoader();
#endif

            if (!loader)
            {
                TRACE_ERROR("Unable to create resource loader for class '{}'", resourceClass->name());
                return nullptr;
            }

            // load the resource
            stream::LoadingContext context;
            context.m_parent = parent;
            context.m_resourceLoader = dependencyLoader;
            context.m_loadImports = dependencyLoader != nullptr;
            context.m_contextName = StringBuf(contextName);
            context.m_baseReferncePath = mountPoint.c_str();
            stream::LoadingResult loadingResult;
            if (!loader->loadObjects(fileStream, context, loadingResult))
            {
                TRACE_ERROR("Unable to deserialize class '{}'", resourceClass->name());
                return nullptr;
            }

            // no objects loaded
            if (loadingResult.m_loadedRootObjects.empty())
            {
                TRACE_ERROR("No objects loaded from file");
                return nullptr;
            }

            // check class
            auto loadedResource = rtti_cast<IResource>(loadingResult.m_loadedRootObjects[0]);
            if (!loadedResource || !loadedResource->is(resourceClass))
            {
                TRACE_ERROR("Invalid resource loaded from class '{}'", resourceClass->name());
                return nullptr;
            }

            // done, return loaded resource
            return loadedResource;
        }

        ObjectPtr LoadSelectiveUncached(const io::AbsolutePath& filePath, ClassType selectiveClassLoad)
        {
            // no selective loading class specified
            if (!selectiveClassLoad)
                return nullptr;

            // get file extension
            auto fileExtension = filePath.fileMainExtension();

            // open file for reading
            auto fileReader = IO::GetInstance().openForReading(filePath);
            if (!fileReader)
            {
                TRACE_ERROR("Unable to open file '{}' for reading", filePath);
                return nullptr;
            }

            // find resource for given extension
            auto resourceClass = IResource::FindResourceClassByExtension(TempString("{}", fileExtension));
            if (!resourceClass)
            {
                TRACE_ERROR("Unable to find resource class for '{}' (unrecognized extension '{}')", filePath, fileExtension);
                return nullptr;
            }

            // create resource loader
            auto loader = resourceClass->findMetadataRef<SerializationLoaderMetadata>().createLoader();
            if (!loader)
            {
                TRACE_ERROR("Unable to create resource loader for class '{}'", resourceClass->name());
                return nullptr;
            }

            // load the resource
            stream::LoadingContext context;
            context.m_resourceLoader = nullptr;
            context.m_loadImports = false;
            context.m_selectiveLoadingClass = selectiveClassLoad;
            context.m_contextName = filePath.ansi_str().c_str();
            stream::LoadingResult loadingResult;
            stream::NativeFileReader fileStream(*fileReader);
            if (!loader->loadObjects(fileStream, context, loadingResult))
            {
                TRACE_ERROR("Unable to deserialize class '{}'", resourceClass->name());
                return nullptr;
            }

            // we should have 1 object
            if (loadingResult.m_loadedRootObjects.size() != 1)
                return nullptr;

            // done, return loaded resource
            auto ret = loadingResult.m_loadedRootObjects[0];
            ASSERT(ret->is(selectiveClassLoad));
            return ret;
        }

        CAN_YIELD bool SaveUncached(const io::AbsolutePath& filePath, const ResourceHandle& data, const ResourceMountPoint& mountPoint)
        {
            // save to buffer
            auto buffer = SaveUncachedToBuffer(data, mountPoint);
            if (!buffer)
                return false;

            // store buffer on disk
            return io::SaveFileFromBuffer(filePath, buffer);
        }

        Buffer SaveUncachedToBuffer(const IResource* data, const ResourceMountPoint& mountPoint)
        {
            // nothing to save
            if (!data)
                return nullptr;

            // create the saver for the cooked resource
            auto saver = data->cls()->findMetadataRef<SerializationSaverMetadata>().createSaver();
            if (!saver)
                return nullptr;

            // save to memory
            stream::MemoryWriter memoryWriter(1024 * 1024);
            stream::SavingContext savingContext(data);
            savingContext.m_baseReferncePath = StringBuf(mountPoint.c_str());
            if (!saver->saveObjects(memoryWriter, savingContext))
                return nullptr;

            // save to disk
            return memoryWriter.extractData();
        }

        //--

        StringBuf SaveUncachedText(const ObjectPtr& data)
        {
            // nothing to save
            if (!data)
                return StringBuf::EMPTY();

            // save to text format
            stream::MemoryWriter writer;
            stream::SavingContext saveContext(data);
            res::text::TextSaver saver;
            if (!saver.saveObjects(writer, saveContext))
            {
                TRACE_ERROR("Failed to save objects to XML");
                return StringBuf::EMPTY();
            }

            // get the text
            return StringBuf((const char*)writer.data());
        }

        ObjectPtr LoadUncachedText(StringView<char> contextName, StringView<char> xmlText, base::res::IResourceLoader* resourceLoader)
        {
            // nothing to load
            if (xmlText.empty())
                return nullptr;


            // load from text format
            stream::MemoryReader reader(xmlText.data(), xmlText.length());
            stream::LoadingContext context;
            context.m_contextName = StringBuf(contextName);
            context.m_resourceLoader = resourceLoader;
            stream::LoadingResult loadingResult;
            res::text::TextLoader loader;
            if (!loader.loadObjects(reader, context, loadingResult))
            {
                TRACE_ERROR("Failed to load objects to XML");
                return nullptr;
            }

            // nothing loaded
            if (loadingResult.m_loadedRootObjects.size() != 1)
            {
                TRACE_ERROR("Invalid number of objects loaded from the XML");
                return nullptr;
            }

            // return the object
            return loadingResult.m_loadedRootObjects[0];
        }

        //--

        Buffer SaveUncachedBinary(const ObjectPtr& object)
        {
            // nothing to save
            if (!object)
                return nullptr;

            // save the collection into memory
            stream::MemoryWriter memoryWriter;
            stream::SavingContext savingContext(object);
            res::binary::BinarySaver saver;
            if (!saver.saveObjects(memoryWriter, savingContext) && !memoryWriter.isError())
            {
                TRACE_ERROR("Failed to save object '{}' 0x{} into binary blob", object->cls()->name(), object.get());
                return nullptr;
            }

            // get the memory
            return memoryWriter.extractData();
        }

        ObjectPtr LoadUncachedBinary(StringView<char> contextName, const Buffer& buffer, const ObjectPtr& newParent /*= nullptr*/)
        {
            // nothing to load
            if (!buffer)
                return nullptr;

            // restore object
            stream::MemoryReader memoryReader(buffer.data(), buffer.size());
            stream::LoadingContext loadingContext;
            loadingContext.m_contextName = StringBuf(contextName);
            stream::LoadingResult loadingResult;
            res::binary::BinaryLoader loader;
            if (!loader.loadObjects(memoryReader, loadingContext, loadingResult) || loadingResult.m_loadedRootObjects.empty())
                return nullptr;

            // get the cloned object
            auto ret = loadingResult.m_loadedRootObjects[0];
            ret->parent(newParent);
            return ret;
        }

        //--

    } // res

    ObjectPtr CloneObjectUntyped(const IObject* object, const IObject* newParent /*= nullptr*/, res::IResourceLoader* resourceLoader /*=nullptr*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
    {
        // nothing to clone
        if (!object)
            return nullptr;

        // save the collection into memory
        stream::MemoryWriter memoryWriter;
        stream::SavingContext savingContext(object);
        auto saver = CreateSharedPtr<res::binary::BinarySaver>();
        if (!saver->saveObjects(memoryWriter, savingContext) && !memoryWriter.isError())
        {
            TRACE_ERROR("Failed to clone object '{}' 0x{}", object->cls()->name(), object);
            return nullptr;
        }

        // find parent loader
        if (nullptr == resourceLoader)
        {
            if (auto* parentResource = object->findParent<base::res::IResource>())
                resourceLoader = parentResource->loader();

            if (newParent)
                if (auto* parentResource = newParent->findParent<base::res::IResource>())
                    resourceLoader = parentResource->loader();
        }

        // restore object
        stream::MemoryReader memoryReader(memoryWriter.data(), memoryWriter.size());
        stream::LoadingContext loadingContext;
        loadingContext.m_rootObjectMutatedClass = mutatedClass;
        loadingContext.m_resourceLoader = resourceLoader;
        stream::LoadingResult loadingResult;
        auto loader = CreateSharedPtr<res::binary::BinaryLoader>();
        if (!loader->loadObjects(memoryReader, loadingContext, loadingResult) || loadingResult.m_loadedRootObjects.empty())
        {
            TRACE_ERROR("Failed to restore clone of object '{}' 0x{}", object->cls()->name(), object);
            return nullptr;
        }

        // get the cloned object
        auto clonedObject = loadingResult.m_loadedRootObjects[0];
        clonedObject->parent(newParent);
        return clonedObject;
    }

    Buffer SaveObjectToBuffer(const IObject* object)
    {
        // nothing to save
        if (!object)
            return Buffer();

        // save the collection into memory
        stream::MemoryWriter memoryWriter;
        stream::SavingContext savingContext(object);
        auto saver = CreateSharedPtr<res::binary::BinarySaver>();
        if (!saver->saveObjects(memoryWriter, savingContext) && !memoryWriter.isError())
        {
            TRACE_ERROR("Failed to save object '{}' 0x{} to buffer", object->cls()->name(), object);
            return nullptr;
        }

        // export data
        return memoryWriter.extractData();
    }

    ObjectPtr LoadObjectFromBuffer(const void* data, uint32_t size, res::IResourceLoader* resourceLoader /*= nullptr*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
    {
        stream::MemoryReader memoryReader(data, size);
        stream::LoadingContext loadingContext;
        loadingContext.m_rootObjectMutatedClass = mutatedClass;
        loadingContext.m_resourceLoader = resourceLoader;
        stream::LoadingResult loadingResult;
        auto loader = CreateSharedPtr<res::binary::BinaryLoader>();
        if (!loader->loadObjects(memoryReader, loadingContext, loadingResult) || loadingResult.m_loadedRootObjects.empty())
        {
            TRACE_ERROR("Failed to restore object from buffer");
            return nullptr;
        }

        // get the cloned object
        if (!loadingResult.m_loadedRootObjects.empty())
            return loadingResult.m_loadedRootObjects[0];
        else
            return nullptr;
    }

    //--

    class ResourceCollector : public base::stream::IDataMapper
    {
    public:
        ResourceCollector(const Array<const IObject*>& objects, bool includeAsync, HashSet<res::ResourceKey>& outList)
            : m_includeAsync(includeAsync)
            , m_collectedResources(outList)
        {
            // remember roots
            m_rootObjects.reserve(objects.size());
            for (const auto* obj : objects)
            {
                m_unprocessed.pushBack(obj);
                m_rootObjects.insert(obj);
            }
        }

        bool processNextObject()
        {
            if (m_unprocessed.empty())
                return false;

            const auto* obj = m_unprocessed.back();
            m_unprocessed.popBack();

            base::stream::NullWriter nullWriter;
            nullWriter.m_mapper = this;
            obj->onWriteBinary(nullWriter);

            return true;
        }

    private:
        bool checkHierarchy(const IObject* obj)
        {
            while (obj)
            {
                if (m_rootObjects.contains(obj))
                    return true;
                obj = obj->parent();
            }

            return false;
        }

        void processObject(const IObject* obj)
        {
            // prevent same object from being visited multiple times
            if (obj && m_visitedObjects.insert(obj))
            {
                // object must be in the same object tree
                if (checkHierarchy(obj))
                {
                    // put the object on the queue
                    m_unprocessed.pushBack(obj);
                }
            }
        }

        virtual void mapName(StringID name, base::stream::MappedNameIndex& outIndex) override {};
        virtual void mapType(Type rttiType, base::stream::MappedTypeIndex& outIndex)  override {};
        virtual void mapProperty(const rtti::Property* rttiProperty, base::stream::MappedPropertyIndex& outIndex) override {};
        virtual void mapBuffer(const Buffer& buffer, base::stream::MappedBufferIndex& outIndex) override {};

        virtual void mapPointer(const IObject* object, base::stream::MappedObjectIndex& outIndex) override
        {
            processObject(object);
        }

        virtual void mapResourceReference(StringView<char> path, ClassType resourceClass, bool async, base::stream::MappedPathIndex& outIndex) override
        {
            if (!path.empty())
            {
                if (m_includeAsync || !async)
                {
                    const auto key = res::ResourceKey(res::ResourcePath(path), resourceClass.cast<res::IResource>());
                    m_collectedResources.insert(key);
                }
            }
        }

    private:
        HashSet<const base::IObject*> m_rootObjects;
        HashSet<const base::IObject*> m_visitedObjects;

        HashSet<res::ResourceKey>& m_collectedResources;

        bool m_includeAsync = false;

        Array<const IObject*> m_unprocessed;
    };

    namespace res
    {
        void ExtractReferencedResources(const Array<const IObject*>& objects, bool includeAsyncLoaded, HashSet<res::ResourceKey>& outReferencedResources)
        {
            ResourceCollector collector(objects, includeAsyncLoaded, outReferencedResources);
            while (collector.processNextObject())
            {
                // TODO: optional fiber yield for long jobs
            }
        }
    } // res

    //--

} // base

