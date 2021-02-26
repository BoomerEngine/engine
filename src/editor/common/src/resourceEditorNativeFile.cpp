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
#include "core/resource/include/resourceLoadingService.h"
#include "engine/ui/include/uiMessageBox.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)
    
//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ResourceEditorNativeFile);
RTTI_END_TYPE();

ResourceEditorNativeFile::ResourceEditorNativeFile(ManagedFileNativeResource* file, ResourceEditorFeatureFlags flags, StringView defaultEditorTag)
    : ResourceEditor(file, flags, defaultEditorTag)
    , m_nativeFile(file)
{
}

ResourceEditorNativeFile::~ResourceEditorNativeFile()
{}

void ResourceEditorNativeFile::handleContentModified()
{

}

void ResourceEditorNativeFile::handleLocalReimport(const res::ResourcePtr& ptr)
{
}

void ResourceEditorNativeFile::applyLocalReimport(const res::ResourcePtr& ptr)
{
    m_resourceEvents.clear();

    if (ptr)
    {
        m_resourceEvents.bind(ptr->eventKey(), EVENT_RESOURCE_MODIFIED) = [this]() {
            m_nativeFile->modify(true);
            handleContentModified();
        };

        m_nativeFile->modify(true);
        m_resource = ptr;

        handleLocalReimport(ptr);

        updateAspects();
    }
}

bool ResourceEditorNativeFile::initialize()
{
    if (!TBaseClass::initialize())
        return false;

    auto content = m_nativeFile->loadContent();
    if (!content)
    {
        TRACE_ERROR("Unable to load resource '{}'", nativeFile()->depotPath());
        return false;
    }

    if (!content->is(nativeFile()->resourceClass()))
    {
        TRACE_ERROR("Loaded resource '{}' is not of expected class '{}' but it a '{}'", nativeFile()->depotPath(), nativeFile()->resourceClass(), content->cls());
        return false;
    }

    m_resource = content;

    m_resourceEvents.clear();
    m_resourceEvents.bind(m_resource->eventKey(), EVENT_RESOURCE_MODIFIED) = [this]() {
        m_nativeFile->modify(true);
        handleContentModified();
    };

    return true;
}

bool ResourceEditorNativeFile::save()
{
    if (!m_nativeFile->storeContent(m_resource))
        return false;

    auto loadingService = GetService<res::LoadingService>();
    auto resoureKey = res::ResourcePath(nativeFile()->depotPath());
    if (!loadingService->loadResource(resoureKey))
    {
        TRACE_WARNING("Reloading of '{}' impossible after save", nativeFile()->depotPath());
    }

    return true;
}

void ResourceEditorNativeFile::cleanup()
{
    m_nativeFile->discardContent();
    m_resource.reset();
    TBaseClass::cleanup();
}

//--

END_BOOMER_NAMESPACE_EX(ed)

