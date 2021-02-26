/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "editor/common/include/resourceEditorNativeFile.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

class MaterialPreviewPanel;
class MaterialPreviewPanel;

/// editor for meshes
class EDITOR_MATERIAL_EDITOR_API MaterialInstanceEditor : public ResourceEditorNativeFile
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialInstanceEditor, ResourceEditorNativeFile);

public:
    MaterialInstanceEditor(ManagedFileNativeResource* file);
    virtual ~MaterialInstanceEditor();

    //--

    INLINE MaterialPreviewPanel* previewPanel() const { return m_previewPanel; }

    INLINE const MaterialInstancePtr& materialInstance() const { return m_instance; }

    //--

private:
    MaterialInstancePtr m_instance;

    RefPtr<MaterialPreviewPanel> m_previewPanel;
    ui::DataInspectorPtr m_properties;

    void createInterface();

    virtual bool initialize() override;
    virtual bool save() override;
};

END_BOOMER_NAMESPACE_EX(ed)
