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
#include "loader.h"

#include "core/object/include/object.h"
#include "core/io/include/fileHandleMemory.h"

BEGIN_BOOMER_NAMESPACE()

Buffer SaveObjectToBuffer(const IObject* object)
{
    if (!object)
        return Buffer();

    FileSavingContext context;
    context.rootObjects.pushBack(object);
    context.format = FileSaveFormat::ProtectedBinaryStream;

    auto writer = RefNew<MemoryWriterFileHandle>();

    FileSavingResult result;
    if (SaveFile(writer, context, result))
        return writer->extract();

    return Buffer();
}

ObjectPtr LoadObjectFromBuffer(const void* data, uint64_t size, bool loadImports/*=false*/)
{
    FileLoadingContext context;
    context.loadImports = loadImports;

    auto reader = RefNew<MemoryAsyncReaderFileHandle>(data, size);

    FileLoadingResult result;
    if (!LoadFile(reader, context, result))
        return nullptr;

    return result.root<IObject>();
}

ObjectPtr CloneObjectUntyped(const IObject* object)
{
    // BIG TODO: implement cloning that does NOT use serialization

    FileSavingContext saveContext;
    saveContext.rootObjects.pushBack(object);
    saveContext.format = FileSaveFormat::ProtectedBinaryStream;

    FileSavingResult saveResult;
    auto writer = RefNew<MemoryWriterFileHandle>();
    if (SaveFile(writer, saveContext, saveResult))
    {
        auto reader = RefNew<MemoryAsyncReaderFileHandle>(writer->extract());

        FileLoadingContext loadContext;
        loadContext.loadImports = true;

        if (auto srcResource = rtti_cast<IResource>(object))
            loadContext.resourceLoadPath = srcResource->loadPath();

        FileLoadingResult result;
        if (LoadFile(reader, loadContext, result))
            return result.root<IObject>();
    }

    return nullptr;
}

END_BOOMER_NAMESPACE()

