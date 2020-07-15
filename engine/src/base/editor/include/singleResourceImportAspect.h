/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

#include "resourceEditor.h"

#include "base/ui/include/uiWindow.h"
#include "base/ui/include/uiDockContainer.h"
#include "base/ui/include/uiDockPanel.h"

namespace ed
{
    ///---

   
    /// file import status widget - check file importability
    class BASE_EDITOR_API AssetImportCheckWidget : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetImportCheckWidget, ui::IElement);
       
    public:
        AssetImportCheckWidget();
        virtual ~AssetImportCheckWidget();

        //---

        // file we are checking
        INLINE const ManagedFile* file() const { return m_file; }

        //---

        // bind the file to do the check for
        void bindFile(const ManagedFile* file);

        //--

    private:
        const ManagedFile* m_file = nullptr;

        ui::TextLabelPtr m_fileNameText;
        ui::TextLabelPtr m_statusText;

        ui::ButtonPtr m_buttonRecheck;
        ui::ButtonPtr m_buttonReimport;
        ui::ButtonPtr m_buttonShowFileList;

        //ui::ListViewPtr m_sourceAssetList;

        void cmdShowSourceAssets();
        void cmdRecheckeck();
        void cmdReimport();
        void cmdSaveConfiguration();
        void cmdLoadConfiguration();
    };


    ///---

    /// import reimport aspect - shows the data about resource import, import config and allows to reimport the asset
    class BASE_EDITOR_API SingleResourceEditorImportAspect: public SingleResourceEditorAspect
    {
        RTTI_DECLARE_VIRTUAL_CLASS(SingleResourceEditorImportAspect, SingleResourceEditorAspect);

    public:
        SingleResourceEditorImportAspect();

        //--

        // get the asset import configuration
        INLINE const res::ResourceConfigurationPtr& config() const { return m_config; }

        //--

    protected:
        virtual bool initialize(SingleResourceEditor* editor);
        virtual void shutdown(); // called when editor is closed

        res::ResourceConfigurationPtr m_config;
        base::SpecificClassType<base::res::ResourceConfiguration> m_configClass;

        bool m_configChanged = false;
    };

    ///---

} // editor

