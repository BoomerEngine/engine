/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/editor/include/singleResourceEditor.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace ed
{

    class MeshPreviewPanel;
    class MeshPreviewPanelWithToolbar;
    class MeshStructurePanel;

    /// editor for meshes
    class ASSETS_MESH_EDITOR_API MeshEditor : public SingleCookedResourceEditor
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshEditor, SingleCookedResourceEditor);

    public:
        MeshEditor(ConfigGroup config, ManagedFile* file);
        virtual ~MeshEditor();

        //--

        INLINE MeshPreviewPanelWithToolbar* previewPanel() const { return m_previewPanel; }

        INLINE rendering::MeshPtr previewMesh() const { return base::rtti_cast<rendering::Mesh>(previewResource().acquire()); }

        //--

    private:
        base::RefPtr<MeshPreviewPanelWithToolbar> m_previewPanel;
        base::RefPtr<MeshStructurePanel> m_structurePanel;

        void createInterface();

        virtual void previewResourceChanged();
    };

} // ed
