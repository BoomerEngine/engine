/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "resourceEditor.h"

namespace ed
{
    ///---

    /// resource editor for native resources
    class EDITOR_COMMON_API ResourceEditorNativeFile : public ResourceEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ResourceEditorNativeFile, ResourceEditor);
        
    public:
        ResourceEditorNativeFile(ManagedFileNativeResource* file, ResourceEditorFeatureFlags flags, StringView defaultEditorTag = "Common");
        virtual ~ResourceEditorNativeFile();

        INLINE ManagedFileNativeResource* nativeFile() const { return m_nativeFile; }
        virtual const res::ResourcePtr& resource() const { return m_resource; }

        virtual bool initialize() override;
        virtual bool save() override;
        virtual void cleanup() override;

        virtual void handleContentModified();
        virtual void handleLocalReimport(const res::ResourcePtr& ptr);

        void applyLocalReimport(const res::ResourcePtr& ptr);

    protected:
        res::ResourcePtr m_resource; // resource being edited
        GlobalEventTable m_resourceEvents;

        ManagedFileNativeResource* m_nativeFile = nullptr;
    };

    ///---
    
} // editor

