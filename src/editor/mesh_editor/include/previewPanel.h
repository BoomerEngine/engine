/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "editor/viewport/include/uiScenePanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

DECLARE_UI_EVENT(EVENT_MATERIAL_CLICKED);

//--

// a preview panel for an image
class EDITOR_MESH_EDITOR_API MeshPreviewPanel : public ui::RenderingSimpleScenePanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshPreviewPanel, ui::RenderingSimpleScenePanel);

public:
    MeshPreviewPanel();
    virtual ~MeshPreviewPanel();

    //---

    // get current settings
    INLINE const MeshPreviewPanelSettings& previewSettings() const { return m_previewSettings; }

    // change preview settings
    void previewSettings(const MeshPreviewPanelSettings& settings);

    // change preview settings
    void changePreviewSettings(const std::function<void(MeshPreviewPanelSettings&)>& func);

    // set preview material
    void previewMaterial(StringID name, MaterialPtr data);

    // set preview mesh
    void previewMesh(const MeshPtr& ptr);

    //--

    virtual void configSave(const ui::ConfigBlock& block) const;
    virtual void configLoad(const ui::ConfigBlock& block);

private:
    MeshPtr m_mesh;

    MeshPreviewPanelSettings m_previewSettings;
    HashMap<StringID, MaterialPtr> m_previewMaterials;

    Array<rendering::ObjectProxyMeshPtr> m_proxies;

    Box m_lastBounds;

    ui::TextLabelPtr m_statsText;

    void destroyPreviewElements();
    void createPreviewElements();

    virtual void handleRender(rendering::FrameParams& frame) override;
    virtual void handlePointSelection(bool ctrl, bool shift, const Point& clientPosition, const Array<Selectable>& selectables) override;
    virtual void handleAreaSelection(bool ctrl, bool shift, const Rect& clientRect, const Array<Selectable>& selectables) override;
    virtual void handlePostRenderContent() override;

    void buildStatsString(IFormatStream& f) const;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
