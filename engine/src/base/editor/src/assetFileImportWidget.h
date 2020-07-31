/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "base/ui/include/uiButton.h"
#include "managedDepot.h"

namespace ed
{

    //--

    // bigger widget that shows more status information about the resource
     class AssetFileImportWidget : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(AssetFileImportWidget, ui::IElement);

    public:
        AssetFileImportWidget();
        virtual ~AssetFileImportWidget();

        void bindFile(ManagedFileNativeResource* file, const res::ResourceConfigurationPtr& config);

    private:
        ManagedFileNativeResource* m_file;

        ManagedFileImportStatusCheckPtr m_checker;
        res::ResourceConfigurationPtr m_config;

        GlobalEventTable m_events;

        ui::EditBoxPtr m_fileNameText;
        ui::TextLabelPtr m_statusText;
        ui::ButtonPtr m_buttonRecheck;
        ui::ButtonPtr m_buttonReimport;
        ui::ButtonPtr m_buttonShowFileList;

        //--

        void cmdShowSourceAssets();
        void cmdRecheckeck();
        void cmdReimport();
        void cmdSaveConfiguration();
        void cmdLoadConfiguration();

        //--

        void updateStatus();
    };

    //--

} // ed