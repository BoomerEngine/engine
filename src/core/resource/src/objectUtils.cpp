/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#include "build.h"
#include "fileSaver.h"
#include "fileLoader.h"
#include "loadingService.h"

#include "core/object/include/object.h"
#include "core/io/include/fileHandleMemory.h"

BEGIN_BOOMER_NAMESPACE()

Buffer SaveObjectToBuffer(const IObject* object)
{
    if (!object)
        return Buffer();

    FileSavingContext context;
    context.rootObject.pushBack(AddRef(object));
    context.protectedStream = false;

    auto writer = RefNew<MemoryWriterFileHandle>();

    if (SaveFile(writer, context))
        return writer->extract();

    return Buffer();
}

ObjectPtr LoadObjectFromBuffer(const void* data, uint64_t size, bool loadImports/*=false*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
{
    ASSERT_EX(mutatedClass == nullptr, "Not used");

    FileLoadingContext context;
    context.loadImports = loadImports;

    auto reader = RefNew<MemoryAsyncReaderFileHandle>(data, size);

    if (LoadFile(reader, context))
        return context.root<IObject>();

    return nullptr;
}

ObjectPtr CloneObjectUntyped(const IObject* object, const IObject* newParent /*= nullptr*/, bool loadImports /*= true*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
{
    FileSavingContext saveContext;
    saveContext.rootObject.pushBack(AddRef(object));
    saveContext.protectedStream = false;

    auto writer = RefNew<MemoryWriterFileHandle>();
    if (SaveFile(writer, saveContext))
    {
        auto reader = RefNew<MemoryAsyncReaderFileHandle>(writer->extract());

        FileLoadingContext loadContext;
        loadContext.loadImports = loadImports;
        loadContext.mutatedRootClass = mutatedClass;

        if (auto srcResource = rtti_cast<IResource>(object))
            loadContext.resourceLoadPath = srcResource->loadPath();

        if (LoadFile(reader, loadContext))
        {
            if (auto loaded = loadContext.root<IObject>())
            {
                loaded->parent(newParent);
                return loaded;
            }
        }
    }

    return nullptr;
}

END_BOOMER_NAMESPACE()

