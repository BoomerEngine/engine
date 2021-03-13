/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#include "build.h"
#include "loader.h"
#include "depot.h"
#include "core/containers/include/path.h"
#include "metadata.h"

BEGIN_BOOMER_NAMESPACE()

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(LoadingService);
RTTI_END_TYPE();

LoadingService::LoadingService()
{}

//--

ResourcePtr LoadResource(StringView path, ClassType expectedClass)
{
    return GetService<LoadingService>()->loadResource(path, expectedClass);
}

ResourcePtr LoadResource(const ResourceID& id, ClassType expectedClass)
{
    return GetService<LoadingService>()->loadResource(id, expectedClass);
}

//--

BaseReference LoadResourceRef(StringView depotPath, ClassType cls)
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

    auto metadataPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);
    auto metadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(metadataPath);
    if (!metadata)
    {
        TRACE_WARNING("Invalid metadata at '{}'", metadataPath);
        return false;
    }

    if (!metadata->resourceClassType.is(cls))
    {
        TRACE_WARNING("Metadata resource class '{}' doest not match target '{}' at '{}'", metadata->resourceClassType, cls, metadataPath);
        return false;
    }

    if (metadata->ids.empty())
    {
        TRACE_WARNING("Metadata '{}' has no IDs", metadataPath);
        return false;
    }

    ResourcePtr data;

    auto resourcePath = ReplaceExtension(depotPath, metadata->loadExtension);
    if (GetService<DepotService>()->fileExists(resourcePath))
    {
        data = LoadResource(resourcePath);
        if (data && !data->is(cls))
        {
            TRACE_WARNING("Loaded file '{}' is of class '{}' and not expected '{}'", resourcePath, data->cls(), cls);
            data.reset();
        }
    }
    else
    {
        TRACE_WARNING("Resource file '{}' pointed by metadata does not exist", resourcePath);
    }

    const auto id = metadata->ids[0];
    return BaseReference(id, data);
}

//--

BaseAsyncReference BuildAsyncResourceRef(StringView depotPath, ClassType cls)
{
    DEBUG_CHECK_RETURN_EX_V(ValidateDepotFilePath(depotPath), "Invalid path", false);

    auto metadataPath = ReplaceExtension(depotPath, ResourceMetadata::FILE_EXTENSION);
    auto metadata = GetService<DepotService>()->loadFileToXMLObject<ResourceMetadata>(metadataPath);
    if (!metadata)
    {
        TRACE_WARNING("Invalid metadata at '{}'", metadataPath);
        return false;
    }

    if (!metadata->resourceClassType.is(cls))
    {
        TRACE_WARNING("Metadata resource class '{}' doest not match target '{}' at '{}'", metadata->resourceClassType, cls, metadataPath);
        return false;
    }

    if (metadata->ids.empty())
    {
        TRACE_WARNING("Metadata '{}' has no IDs", metadataPath);
        return false;
    }

    ResourcePtr data;

    auto resourcePath = ReplaceExtension(depotPath, metadata->loadExtension);
    if (!GetService<DepotService>()->fileExists(resourcePath))
    {
        TRACE_WARNING("Resource file '{}' pointed by metadata does not exist", resourcePath);
    }

    const auto id = metadata->ids[0];
    return BaseAsyncReference(id);
}

//--

END_BOOMER_NAMESPACE()