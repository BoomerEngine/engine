/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editors #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

class AssetFileImportWidget;

/// reimport panel for resources that were imported
class EDITOR_ASSETS_API ResourceReimportPanel : public ui::DockPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(ResourceReimportPanel, ui::DockPanel);

public:
    ResourceReimportPanel(ResourceEditor* editor);

    void updateMetadata(ResourceMetadata* metadata);

protected:
    ResourceEditor* m_editor = nullptr;

    ResourceConfigurationPtr m_mergedBaseConfig; // base + system config
    ResourceConfigurationPtr m_userConfig;

    RefPtr<AssetFileImportWidget> m_importWidget;
    ui::DataInspectorPtr m_dataInspector;

    void inplaceReimport();

    bool m_configChanged = false;
};

///---

END_BOOMER_NAMESPACE_EX(ed)

