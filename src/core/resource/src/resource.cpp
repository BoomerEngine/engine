/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#include "build.h"
#include "resource.h"
#include "loader.h"
#include "classLookup.h"
#include "metadata.h"
#include "path.h"
#include "tags.h"

BEGIN_BOOMER_NAMESPACE()

//---

static std::atomic<ResourceUniqueID> GResourceUniqueID(1);

//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IResource);
    RTTI_METADATA(ResourceDataVersionMetadata).version(0);
    RTTI_PROPERTY(m_metadata);
RTTI_END_TYPE();

IResource::IResource()
    : m_runtimeUniqueId(++GResourceUniqueID)
    , m_runtimeVersion(0)
    , m_loader(nullptr)
{}

IResource::~IResource()
{
    /*if (auto loader = m_loader.lock())
    {
        TRACE_INFO("Destroying resource 0x{} '{}'", Hex(this), path());
        loader->notifyResourceUnloaded(path());
    }*/
}

//DispatchGlobalEvent(eventKey(), EVENT_RESOURCE_RELOADED, m_reloadedData);

void IResource::metadata(const ResourceMetadataPtr& data)
{
    DEBUG_CHECK_EX(!data->parent(), "Metadata is already parented");
    m_metadata = data;
    m_metadata->parent(this);
}

void IResource::markModified()
{
    // invalidate runtime version of the resource, this may cause refresh of the preview panels or other cached data
    invalidateRuntimeVersion();

    // this resource may be contained inside other file, we still need to propagate
    TBaseClass::markModified();

    // mark as modified only if we are standalone resource
    {
        auto selfRef = ResourcePtr(AddRef(this));
        DispatchGlobalEvent(eventKey(), EVENT_RESOURCE_MODIFIED, selfRef);
    }
}

void IResource::invalidateRuntimeVersion()
{
    m_runtimeVersion += 1;
}

//--

SpecificClassType<IResource> IResource::FindResourceClassByHash(ResourceClassHash hash)
{
    return prv::ResourceClassLookup::GetInstance().resolveResourceClassHash(hash);
}

SpecificClassType<IResource> IResource::FindResourceClassByExtension(StringView extension)
{
    return prv::ResourceClassLookup::GetInstance().resolveResourceExtension(extension);
}

SpecificClassType<IResource> IResource::FindResourceClassByPath(StringView path)
{
    if (path.empty())
        return nullptr;

    auto ext = path.afterFirst(".");
    if (ext.empty())
        return nullptr;

    return FindResourceClassByExtension(ext);
}

StringBuf IResource::GetResourceDescriptionForClass(ClassType resourceClass)
{
    if (!resourceClass)
        return StringBuf("None");

    auto descMetaData  = resourceClass->findMetadata<ResourceDescriptionMetadata>();
    if (descMetaData && descMetaData->description() && *descMetaData->description())
        return StringBuf(descMetaData->description());

    if (resourceClass->shortName())
        return StringBuf(resourceClass->shortName().view());

    return StringBuf(resourceClass->name().view().afterLast("::"));
}

StringBuf IResource::GetResourceExtensionForClass(ClassType resourceClass)
{
    if (!resourceClass)
        return StringBuf::EMPTY();

    auto extMetaData = resourceClass->findMetadata<ResourceExtensionMetadata>();
    if (extMetaData)
        return StringBuf(extMetaData->extension());

    return StringBuf::EMPTY();
}

void IResource::discardEditorData()
{
}

bool IResource::calcResourceStreamingDistance(float& outDistance) const
{
    return false;
}

void IResource::bindLoadPath(StringView path)
{
    m_loadPath = StringBuf(path);
}

//--

END_BOOMER_NAMESPACE()
