/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "engine/ui/include/uiButton.h"
#include "managedDepot.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

// micro-indicator for showing if imported file is up to date
class AssetFileSmallImportIndicator : public ui::Button
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetFileSmallImportIndicator, ui::Button);

public:
    AssetFileSmallImportIndicator(StringView depotPath);
    virtual ~AssetFileSmallImportIndicator();

    //--

    void recheck();

private:
    StringView m_depotPath;

    AssetImportStatusCheckPtr m_checker;

    ui::TextLabelPtr m_caption;

    GlobalEventTable m_events;

    //--       

    void showContextMenu();
    void updateStatus();
};

//--

END_BOOMER_NAMESPACE_EX(ed)
