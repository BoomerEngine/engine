/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\serialization #]
***/

#include "build.h"
#include "resourceFileSaver.h"
#include "resourceFileLoader.h"
#include "resourceLoadingService.h"

#include "base/object/include/object.h"
#include "base/io/include/ioFileHandleMemory.h"

BEGIN_BOOMER_NAMESPACE(base)

Buffer SaveObjectToBuffer(const IObject* object)
{
    if (!object)
        return Buffer();

    res::FileSavingContext context;
    context.rootObject.pushBack(AddRef(object));
    context.protectedStream = false;

    auto writer = base::RefNew<io::MemoryWriterFileHandle>();

    if (SaveFile(writer, context))
        return writer->extract();

    return Buffer();
}

ObjectPtr LoadObjectFromBuffer(const void* data, uint64_t size, res::ResourceLoader* loader /*= nullptr*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
{
    ASSERT_EX(mutatedClass == nullptr, "Not used");

    res::FileLoadingContext context;
    context.resourceLoader = loader;

    auto reader = base::RefNew<io::MemoryAsyncReaderFileHandle>(data, size);

    if (LoadFile(reader, context))
        return context.root<IObject>();

    return nullptr;
}

ObjectPtr CloneObjectUntyped(const IObject* object, const IObject* newParent /*= nullptr*/, res::ResourceLoader* loader /*= nullptr*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
{
    res::FileSavingContext saveContext;
    saveContext.rootObject.pushBack(AddRef(object));
    saveContext.protectedStream = false;

    if (!loader)
        loader = GetService<res::LoadingService>()->loader();

    auto writer = base::RefNew<io::MemoryWriterFileHandle>();
    if (SaveFile(writer, saveContext))
    {
        auto reader = base::RefNew<io::MemoryAsyncReaderFileHandle>(writer->extract());

        res::FileLoadingContext loadContext;
        loadContext.resourceLoader = loader;
        loadContext.mutatedRootClass = mutatedClass;

        if (auto srcResource = rtti_cast<res::IResource>(object))
            loadContext.resourceLoadPath = srcResource->path();

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

END_BOOMER_NAMESPACE(base)

