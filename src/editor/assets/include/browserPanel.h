/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: assets #]
***/

#pragma once

#include "editor/common/include/mainPanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///--

/// assert browser tab
class EDITOR_ASSETS_API AssetBrowserPanel : public IEditorPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(AssetBrowserPanel, IEditorPanel);

public:
    AssetBrowserPanel();
    virtual ~AssetBrowserPanel();

    //--

    // get selected file
    StringBuf selectedFile() const;

    // get current directory
    StringBuf currentDirectory() const;

    //--

    // show file in the asset browser
    bool showFile(StringView depotPath);

    // show directory in the depot tree and possible also as a file list
    void showDirectory(StringView depotDirPath);

    //--

private:
    GlobalEventTable m_depotEvents;

    ui::DockContainerPtr m_dockArea;

    RefPtr<AssetBrowserTreePanel> m_treePanel;

    //--

    void navigateToDirectory(StringView depotPath, RefPtr<AssetBrowserTabFiles>* outTab = nullptr);
    void closeDirectoryTab(StringView depotPath);

    virtual bool handleEditorClose() override;

    virtual void configLoad(const ui::ConfigBlock& block) override;
    virtual void configSave(const ui::ConfigBlock& block) const override;

    //--
};

///--

END_BOOMER_NAMESPACE_EX(ed)


