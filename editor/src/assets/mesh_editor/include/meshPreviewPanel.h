/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "rendering/ui_viewport/include/renderingFullScenePanel.h"

namespace ed
{
    //--

    DECLARE_UI_EVENT(EVENT_MATERIAL_CLICKED);

    class  MeshPreview;

    struct MeshPreviewPanelSettings
    {
        int forceLod = -1;

        bool showBounds = false;
        bool showSkeleton = false;
        bool showBoneNames = false;
        bool showBoneAxes = false;

        bool isolateMaterials = false;
        bool highlightMaterials = false;
        base::HashSet<base::StringID> selectedMaterials;
    };

    //--

    // a preview panel for an image
    class ASSETS_MESH_EDITOR_API MeshPreviewPanel : public ui::RenderingFullScenePanelWithScene
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshPreviewPanel, ui::RenderingFullScenePanelWithScene);

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
        void previewMaterial(base::StringID name, rendering::MaterialPtr data);

        // set preview mesh
        void previewMesh(const rendering::MeshPtr& ptr);

        //--

        virtual void configSave(const ui::ConfigBlock& block) const;
        virtual void configLoad(const ui::ConfigBlock& block);

    private:
        rendering::MeshPtr m_mesh;

        MeshPreviewPanelSettings m_previewSettings;
        base::HashMap<base::StringID, rendering::MaterialPtr> m_previewMaterials;

        base::Array<rendering::scene::ObjectProxyPtr> m_proxies;

        base::Box m_lastBounds;

        void destroyPreviewElements();
        void createPreviewElements();

        virtual void handleRender(rendering::scene::FrameParams& frame) override;
        virtual void handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual void handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables) override;
    };

    //--

} // ed