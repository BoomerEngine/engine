/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "editor/assets/include/resourceEditor.h"
#include "engine/mesh/include/mesh.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

class MeshPreviewPanel;
class MeshStructurePanel;
class MeshMaterialsPanel;

/// editor for meshes
class EDITOR_MESH_EDITOR_API MeshEditor : public ResourceEditor
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshEditor, ResourceEditor);

public:
    MeshEditor(const ResourceInfo& info);
    virtual ~MeshEditor();

    //--

    INLINE MeshPreviewPanel* previewPanel() const { return m_previewPanel; }

    //--

    virtual void fillViewMenu(ui::MenuButtonContainer* menu) override;

private:
    RefPtr<MeshPreviewPanel> m_previewPanel;
    RefPtr<MeshStructurePanel> m_structurePanel;
    RefPtr<MeshMaterialsPanel> m_materialsPanel;

    MeshPreviewPanelSettings m_previewSettings;

    ui::ComboBoxPtr m_lodList;

    bool m_hasDefaultCamera = false;
    Vector3 m_defaultCameraPosition;
    Angles m_defaultCameraRotation;

    void createInterface();
    
    void updateMaterialHighlights();
    void updateToolbar();
    void updateLODList();
    void updatePreviewSettings();

    virtual void reimported(ResourcePtr resource, ResourceMetadataPtr metadata) override;
};

END_BOOMER_NAMESPACE_EX(ed)
