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
#include "engine/ui/include/uiListViewEx.h"

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
class MeshMaterialParameters : public ui::IListItem
{
    RTTI_DECLARE_VIRTUAL_CLASS(MeshMaterialParameters, ui::IListItem);

public:
    MeshMaterialParameters(MaterialInstance* data, StringID name);

    INLINE StringID name() const { return m_name; }

    INLINE const MaterialInstancePtr& data() const { return m_data; }

    void updateDisplayString();
    void baseMaterial(const MaterialRef& material);

private:
    GlobalEventTable m_events;

    StringID m_name;

    ui::TextLabelPtr m_label;

    MaterialInstancePtr m_data;

    //--

    virtual bool handleItemFilter(const ui::ICollectionView* view, const ui::SearchPattern& filter) const override final;
    virtual void handleItemSort(const ui::ICollectionView* view, int colIndex, SortingData& outInfo) const override final;
    virtual bool handleItemContextMenu(ui::ICollectionView* view, const ui::CollectionItems& items, const ui::Position& pos, input::KeyMask controlKeys) override final;
    virtual bool handleItemActivate(ui::ICollectionView* view) override final;

    virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override final;
    virtual void handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override final;
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

    StringID selectedMaterial() const;

    void bindResource(const MeshPtr& mesh);

    void showMaterials(const Array<StringID>& names);

    void collectSelectedMaterialNames(HashSet<StringID>& outNames) const;

private:
    MeshPtr m_mesh;

    //--

    ui::DataInspectorPtr m_properties;
    ui::ListViewExPtr m_list;
    ui::ToolBarPtr m_toolbar;
        
    MeshMaterialPreviewSettings m_settings;

    //--
         
    void refreshMaterialList();
    void refreshMaterialProperties();

    void updateToolbar();
};

//--

END_BOOMER_NAMESPACE_EX(ed)
