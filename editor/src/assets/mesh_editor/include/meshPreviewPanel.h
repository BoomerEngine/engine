/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "rendering/ui/include/renderingFullScenePanel.h"

namespace ed
{
    //--

    class  MeshPreview;

    struct MeshPreviewPanelSettings
    {
        int forceLod = -1;

        bool showBounds = false;
        bool showSkeleton = false;
        bool showBoneNames = false;
        bool showBoneAxes = false;

        base::HashSet<base::StringID> highlightedMaterials;
    };

    //--

    // a preview panel for an image
    class ASSETS_MESH_EDITOR_API MeshPreviewPanel : public ui::RenderingFullScenePanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshPreviewPanel, ui::RenderingFullScenePanel);

    public:
        MeshPreviewPanel();
        virtual ~MeshPreviewPanel();

        //---

        // get current settings
        INLINE const MeshPreviewPanelSettings& previewSettings() const { return m_previewSettings; }

        // change preview settings
        void previewSettings(const MeshPreviewPanelSettings& settings);

        // set preview material
        void previewMaterial(base::StringID name, rendering::MaterialPtr data);

        // set preview mesh
        void previewMesh(const rendering::MeshPtr& ptr);

        //--

    private:
        rendering::MeshPtr m_mesh;

        MeshPreviewPanelSettings m_previewSettings;
        base::HashMap<base::StringID, rendering::MaterialPtr> m_previewMaterials;

        rendering::scene::ProxyHandle m_mainProxy;

        void destroyPreviewElements();
        void createPreviewElements();

        virtual void handleRender(rendering::scene::FrameParams& frame) override;
    };

    //--

} // ed