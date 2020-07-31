/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#include "build.h"
#include "resourceEditorNativeFile.h"
#include "managedFile.h"
#include "managedFileNativeResource.h"

namespace ed
{
    
    //---

    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ResourceEditorNativeFile);
    RTTI_END_TYPE();

    ResourceEditorNativeFile::ResourceEditorNativeFile(ManagedFileNativeResource* file, ResourceEditorFeatureFlags flags)
        : ResourceEditor(file, flags)
        , m_nativeFile(file)
    {}

    ResourceEditorNativeFile::~ResourceEditorNativeFile()
    {}

    void ResourceEditorNativeFile::bindResource(const res::ResourcePtr& resource)
    {
        if (m_resource)
            m_resourceEvents.unbind(m_resource->eventKey());

        m_resource = resource;
        m_nativeFile->modify(false);

        for (auto& aspect : aspects())
            aspect->resourceChanged();

        // in case the edited resource get's reloaded rebind this editor to the new resource
        // NOTE: handled in the file itself
        /*m_resourceEvents.bind(m_resource->eventKey(), EVENT_RESOURCE_RELOADED) = [this](res::ResourcePtr reloadedResource)
        {
            bindResource(reloadedResource);
        };*/
    }

    bool ResourceEditorNativeFile::save()
    {
        return m_nativeFile->storeContent();
    }

    void ResourceEditorNativeFile::cleanup()
    {
        m_nativeFile->discardContent();
        m_resource.reset();
        TBaseClass::cleanup();
    }

    //--

} // editor

