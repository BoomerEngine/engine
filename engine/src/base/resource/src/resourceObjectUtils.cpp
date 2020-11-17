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
#include "base/object/include/object.h"
#include "base/io/include/ioFileHandleMemory.h"

namespace base
{

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

    ObjectPtr LoadObjectFromBuffer(const void* data, uint64_t size, res::IResourceLoader* loader /*= nullptr*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
    {
        ASSERT_EX(mutatedClass == nullptr, "Not used");

        res::FileLoadingContext context;
        context.resourceLoader = loader;

        auto reader = base::RefNew<io::MemoryAsyncReaderFileHandle>(data, size);

        if (LoadFile(reader, context))
            return context.root<IObject>();

        return nullptr;
    }

    ObjectPtr CloneObjectUntyped(const IObject* object, const IObject* newParent /*= nullptr*/, res::IResourceLoader* loader /*= nullptr*/, SpecificClassType<IObject> mutatedClass /*= nullptr*/)
    {
        res::FileSavingContext saveContext;
        saveContext.rootObject.pushBack(AddRef(object));
        saveContext.protectedStream = false;

        auto writer = base::RefNew<io::MemoryWriterFileHandle>();
        if (SaveFile(writer, saveContext))
        {
            auto reader = base::RefNew<io::MemoryAsyncReaderFileHandle>(writer->extract());

            res::FileLoadingContext loadContext;
            loadContext.resourceLoader = loader;
            loadContext.mutatedRootClass = mutatedClass;

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

} // base