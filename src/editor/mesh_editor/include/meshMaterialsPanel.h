/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiSimpleListModel.h"
#include "engine/ui/include/uiAbstractItemModel.h"
#include "engine/ui/include/uiDragDrop.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

class MeshPreviewPanelWithToolbar;
class MeshEditorMaterialInstance;

//--

struct MeshMaterialPreviewSettings
{
    bool highlight = false;
    bool isolate = false;
};

//--

/// helper object for editable mesh material
class MeshMaterialParameters : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshMaterialParameters, IObject);

public:
    MeshMaterialParameters(MaterialInstance* data, StringID name);

    INLINE StringID name() const { return m_name; }

    INLINE const MaterialInstancePtr& data() const { return m_data; }

    INLINE const StringBuf& displayString() const { return m_displayString; }

    bool updateDisplayString();
    bool baseMaterial(const MaterialRef& material);

private:
    StringID m_name;
    StringBuf m_displayString;

    MaterialInstancePtr m_data;
};

//--

/// list model for material list
class MeshMaterialListModel : public ui::SimpleTypedListModel<RefPtr<MeshMaterialParameters>>
{
public:
    MeshMaterialListModel();

    virtual bool compare(const RefPtr<MeshMaterialParameters>& a, const RefPtr<MeshMaterialParameters>& b, int colIndex) const override final;
    virtual bool filter(const RefPtr<MeshMaterialParameters>& data, const ui::SearchPattern& filter, int colIndex = 0) const override final;
    virtual StringBuf content(const RefPtr<MeshMaterialParameters>& data, int colIndex = 0) const override final;

    virtual ui::DragDropHandlerPtr handleDragDropData(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data, const ui::Position& pos) override final;
    virtual bool handleDragDropCompletion(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data) override final;

    StringID materialName(const ui::ModelIndex& index) const;

    ui::ModelIndex findMaterial(StringID name) const;
};

//--

DECLARE_UI_EVENT(EVENT_MATERIAL_SELECTION_CHANGED);

/// editor aspect for displaying the mesh packing parameters
class MeshMaterialsPanel : public ui::IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshMaterialsPanel, ui::IElement);

public:
    MeshMaterialsPanel(ActionHistory* actionHistory);

    INLINE const MeshMaterialPreviewSettings& settings() const { return m_settings; }

    void bindResource(const MeshPtr& mesh);

    void showMaterials(const Array<StringID>& names);

    void collectSelectedMaterialNames(HashSet<StringID>& outNames) const;
               
private:
    MeshPtr m_mesh;

    //--

    ui::DataInspectorPtr m_properties;
    ui::ListViewPtr m_list;
        
    MeshMaterialPreviewSettings m_settings;

    ui::Timer m_captionsRefreshTimer;

    RefPtr<MeshMaterialListModel> m_listModel;

    void refreshMaterialList();
    void refreshMaterialProperties();
    void updateCaptions();
};

//--

END_BOOMER_NAMESPACE_EX(ed)
