/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/editor/include/resourceEditorNativeFile.h"
#include "rendering/mesh/include/renderingMesh.h"

namespace ed
{

    class MeshPreviewPanel;
    class MeshStructurePanel;
    class MeshMaterialsPanel;

    /// editor for meshes
    class ASSETS_MESH_EDITOR_API MeshEditor : public ResourceEditorNativeFile
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshEditor, ResourceEditorNativeFile);

    public:
        MeshEditor(ManagedFileNativeResource* file);
        virtual ~MeshEditor();

        //--

        INLINE MeshPreviewPanel* previewPanel() const { return m_previewPanel; }

        //--

        virtual void fillViewMenu(ui::MenuButtonContainer* menu) override;

    private:
        base::RefPtr<MeshPreviewPanel> m_previewPanel;
        base::RefPtr<MeshStructurePanel> m_structurePanel;
        base::RefPtr<MeshMaterialsPanel> m_materialsPanel;

        bool m_hasDefaultCamera = false;
        base::Vector3 m_defaultCameraPosition;
        base::Angles m_defaultCameraRotation;

        void createInterface();
        void updateMaterialHighlights();

        virtual bool initialize() override;
        
        virtual void handleLocalReimport(const res::ResourcePtr& ptr) override;
    };

} // ed
