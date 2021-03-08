/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource #]
***/

#pragma once

#include "core/containers/include/hashSet.h"
#include "core/object/include/object.h"

BEGIN_BOOMER_NAMESPACE()

//-----

// Base resource class
class CORE_RESOURCE_API IResource : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IResource, IObject);

public:
    //--

    static inline StringView FILE_EXTENSION = "xfile";

    //--

    // File path used to load the resource - NOTE, debug only
    INLINE const StringBuf& loadPath() const { return m_loadPath; }

    // Get runtime only unique ID, allows to identify resources by simpler means than just the number
    // NOTE: the ID gets reset if the resource is reloaded
    INLINE ResourceUniqueID runtimeUniqueId() const { return m_runtimeUniqueId; }

    // Get the resource internal version, changed every time the markModified is called or resource is reloaded
    // Can be used to track global changed on the resource and for example refresh the preview, etc
    INLINE ResourceRuntimeVersion runtimeVersion() const { return m_runtimeVersion; }

    // Get resource metadata, NOTE: valid only for binary resources, can be loaded separately
    INLINE const ResourceMetadataPtr& metadata() const { return m_metadata; }

    // Is the resource considered modified ?
    INLINE bool modified() const { return m_modified; }

    //--

    IResource();
    virtual ~IResource();

    // mark resource as modified
    // NOTE: this propagates the event to the loader
    // NOTE: the isModified flag is NOT stored in the resource but on the side of the managing structure (like ManagedDepot)
    virtual void markModified() override;

    // reset resource modified flag
    void resourceModifiedFlag();

    // invalidate runtime version of the resource, may force users to refresh preview
    void invalidateRuntimeVersion();

    // bind source load path
    void bindLoadPath(StringView path);

    //---

    // Get resource description for given class
    static StringBuf GetResourceDescriptionForClass(ClassType resourceClass);

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
    void metadata(const ResourceMetadataPtr& data);

    //--

private:
    StringBuf m_loadPath; // path we loaded this resource from, debug only

    ResourceUniqueID m_runtimeUniqueId = 0; // resource runtime unique ID, can be used to index maps instead of pointer
    ResourceRuntimeVersion m_runtimeVersion = 0; // internal runtime version of the resource, can be observed and a callback can be attached

    ResourceMetadataPtr m_metadata; // source dependencies and stuff
    ResourcePtr m_reloadedData; // new version of this resource

    bool m_modified = false;

    //--
};

//---

END_BOOMER_NAMESPACE()
