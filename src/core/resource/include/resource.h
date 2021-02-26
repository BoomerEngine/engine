/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once

#include "resourcePath.h"

#include "core/containers/include/hashSet.h"
#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE_EX(res)

//-----

// Base resource class
class CORE_RESOURCE_API IResource : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IResource, IObject);

public:
    // Get the resource path used to load this resource (it's an absolute depot path, ie. "/engine/textures/lena.png")
    INLINE const ResourcePath& path() const { return m_path; }

    // Get runtime only unique ID, allows to identify resources by simpler means than just the number
    // NOTE: the ID gets reset if the resource is reloaded
    INLINE ResourceUniqueID runtimeUniqueId() const { return m_runtimeUniqueId; }

    // Get the resource internal version, changed every time the markModified is called or resource is reloaded
    // Can be used to track global changed on the resource and for example refresh the preview, etc
    INLINE ResourceRuntimeVersion runtimeVersion() const { return m_runtimeVersion; }

    // Get resource metadata, NOTE: valid only for binary resources, can be loaded separately
    INLINE const MetadataPtr& metadata() const { return m_metadata; }

    //--

    IResource();
    virtual ~IResource();

    // mark resource as modified
    // NOTE: this propagates the event to the loader
    // NOTE: the isModified flag is NOT stored in the resource but on the side of the managing structure (like ManagedDepot)
    virtual void markModified() override;

    // invalidate runtime version of the resource, may force users to refresh preview
    void invalidateRuntimeVersion();

    // Bind source loader data for this resource
    void bindToLoader(ResourceLoader* loader, const ResourcePath& loadPath);

    //---

    // Find resource class based on the class hash
    static SpecificClassType<IResource> FindResourceClassByHash(ResourceClassHash hash);

    // Find resource class based on resource extension
    static SpecificClassType<IResource> FindResourceClassByExtension(StringView extension);

    // Find resource class based on resource path
    static SpecificClassType<IResource> FindResourceClassByPath(StringView path);

    // Get resource description for given class
    static StringBuf GetResourceDescriptionForClass(ClassType resourceClass);

    // Get resource extension for given class
    static StringBuf GetResourceExtensionForClass(ClassType resourceClass);

    //---

    // Drop any editor only data for this resource
    // Called when building deployable packages
    virtual void discardEditorData();

    //---

    // Retrieve the loading (streaming) distance for this resource
    // NOTE: this is implementation specific
    // NOTE: returns false if there's no well determined streaming distance
    virtual bool calcResourceStreamingDistance(float& outDistance) const;

    //--

    // Bind new metadata
    void metadata(const MetadataPtr& data);

    //--

private:
    ResourcePath m_path; // path we loaded this resource from
    RefWeakPtr<ResourceLoader> m_loader; // loader used to load this resource, NOTE: not set if never loaded

    ResourceUniqueID m_runtimeUniqueId; // resource runtime unique ID, can be used to index maps instead of pointer
    ResourceRuntimeVersion m_runtimeVersion; // internal runtime version of the resource, can be observed and a callback can be attached

    MetadataPtr m_metadata; // source dependencies and stuff
    ResourceHandle m_reloadedData; // new version of this resource

    //--
};

//---

END_BOOMER_NAMESPACE_EX(res)
