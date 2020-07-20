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


    ///---

    /// import reimport aspect - shows the data about resource import, import config and allows to reimport the asset
    class BASE_EDITOR_API ResourceEditorNativeImportAspect : public IResourceEditorAspect
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ResourceEditorNativeImportAspect, IResourceEditorAspect);

    public:
        ResourceEditorNativeImportAspect();

        //--

        // get the asset import configuration
        INLINE const res::ResourceConfigurationPtr& config() const { return m_config; }

        //--

    protected:
        virtual bool initialize(ResourceEditor* editor);
        virtual void close(); // called when editor is closed

        res::ResourceConfigurationPtr m_config;

        bool m_configChanged = false;
    };

    ///---

} // editor

