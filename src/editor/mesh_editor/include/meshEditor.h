/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "editor/common/include/resourceEditorNativeFile.h"
#include "engine/mesh/include/renderingMesh.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

class MeshPreviewPanel;
class MeshStructurePanel;
class MeshMaterialsPanel;

/// editor for meshes
class EDITOR_MESH_EDITOR_API MeshEditor : public ResourceEditorNativeFile
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
    RefPtr<MeshPreviewPanel> m_previewPanel;
    RefPtr<MeshStructurePanel> m_structurePanel;
    RefPtr<MeshMaterialsPanel> m_materialsPanel;

    bool m_hasDefaultCamera = false;
    Vector3 m_defaultCameraPosition;
    Angles m_defaultCameraRotation;

    void createInterface();
    void updateMaterialHighlights();

    virtual bool initialize() override;
        
    virtual void handleLocalReimport(const res::ResourcePtr& ptr) override;
};

END_BOOMER_NAMESPACE_EX(ed)
