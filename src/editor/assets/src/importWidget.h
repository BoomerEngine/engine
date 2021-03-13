/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiButton.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

DECLARE_UI_EVENT(EVENT_RESOURCE_REIMPORT_WITH_CONFIG, ResourceConfigurationPtr);

//--

// bigger widget that shows more status information about the resource
class AssetFileImportWidget : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetFileImportWidget, ui::IElement);

public:
    AssetFileImportWidget();
    virtual ~AssetFileImportWidget();

    void bindFile(StringView depotPath, const ResourceConfigurationPtr& config);

private:
    StringBuf m_depotPath;

    AssetImportStatusCheckPtr m_checker;
    ResourceConfigurationPtr m_config;

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

END_BOOMER_NAMESPACE_EX(ed)
