/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "meshPreviewPanel.h"

namespace ed
{
    //--

    class MeshPreviewElement_FullMesh;

    //--

    // a preview panel for the mesh, contains toolbar
    class ASSETS_MESH_EDITOR_API MeshPreviewPanelWithToolbar : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshPreviewPanelWithToolbar, ui::IElement);

    public:
        MeshPreviewPanelWithToolbar();
        virtual ~MeshPreviewPanelWithToolbar();

        //--

        // current settings
        const MeshPreviewPanelSettings& previewSettings() const;

        // change preview settings
        void previewSettings(const MeshPreviewPanelSettings& settings);

        // set preview material
        void previewMaterial(base::StringID name, const rendering::MaterialPtr& data);

        // set the preview mesh
        void previewMesh(const rendering::MeshPtr& mesh);

    private:
        ui::ToolBarPtr m_toolbar;
        base::RefPtr<MeshPreviewPanel> m_previewPanel;

        rendering::MeshPtr m_mesh;
    };

    //--

} // ed