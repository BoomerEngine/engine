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
    class BASE_EDITOR_API ResourceEditorNativeFile : public ResourceEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ResourceEditorNativeFile, ResourceEditor);
        
    public:
        ResourceEditorNativeFile(ConfigGroup typeConfig, ManagedFileNativeResource* file, ResourceEditorFeatureFlags flags);
        virtual ~ResourceEditorNativeFile();

        INLINE ManagedFileNativeResource* nativeFile() const { return m_nativeFile; }

        INLINE const res::ResourcePtr& resource() const { return m_resource; }

        virtual void bindResource(const res::ResourcePtr& resource);
        virtual bool save() override;
        virtual void close() override;

    protected:
        res::ResourcePtr m_resource; // resource being edited
        GlobalEventTable m_resourceEvents;

        ManagedFileNativeResource* m_nativeFile = nullptr;
    };

    ///---
    
} // editor

