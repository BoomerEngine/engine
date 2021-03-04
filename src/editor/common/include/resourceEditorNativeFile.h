/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "resourceEditor.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

/// resource editor for native resources
class EDITOR_COMMON_API ResourceEditorNativeFile : public ResourceEditor
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceEditorNativeFile, ResourceEditor);
        
public:
    ResourceEditorNativeFile(ManagedFileNativeResource* file, ResourceEditorFeatureFlags flags, StringView defaultEditorTag = "Common");
    virtual ~ResourceEditorNativeFile();

    INLINE ManagedFileNativeResource* nativeFile() const { return m_nativeFile; }
    virtual const ResourcePtr& resource() const { return m_resource; }

    virtual bool initialize() override;
    virtual bool save() override;
    virtual void cleanup() override;

    virtual void handleContentModified();
    virtual void handleLocalReimport(const ResourcePtr& ptr);

    void applyLocalReimport(const ResourcePtr& ptr);

protected:
    ResourcePtr m_resource; // resource being edited
    GlobalEventTable m_resourceEvents;

    ManagedFileNativeResource* m_nativeFile = nullptr;
};

///---
    
END_BOOMER_NAMESPACE_EX(ed)
