/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "base/ui/include/uiSimpleListModel.h"
#include "base/ui/include/uiAbstractItemModel.h"
#include "base/ui/include/uiDragDrop.h"

namespace ed
{
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
    class MeshMaterialParameters : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshMaterialParameters, base::IObject);

    public:
        MeshMaterialParameters(rendering::MaterialInstance* data, base::StringID name);

        INLINE base::StringID name() const { return m_name; }

        INLINE const rendering::MaterialInstancePtr& data() const { return m_data; }

        INLINE const base::StringBuf& displayString() const { return m_displayString; }

        bool updateDisplayString();
        bool baseMaterial(const rendering::MaterialRef& material);

    private:
        base::StringID m_name;
        base::StringBuf m_displayString;

        rendering::MaterialInstancePtr m_data;
    };

    //--

    /// list model for material list
    class MeshMaterialListModel : public ui::SimpleTypedListModel<base::RefPtr<MeshMaterialParameters>>
    {
    public:
        MeshMaterialListModel();

        virtual bool compare(const base::RefPtr<MeshMaterialParameters>& a, const base::RefPtr<MeshMaterialParameters>& b, int colIndex) const override final;
        virtual bool filter(const base::RefPtr<MeshMaterialParameters>& data, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf content(const base::RefPtr<MeshMaterialParameters>& data, int colIndex = 0) const override final;

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
        MeshMaterialsPanel(base::ActionHistory* actionHistory);

        INLINE const MeshMaterialPreviewSettings& settings() const { return m_settings; }

        void bindResource(const rendering::MeshPtr& mesh);

        void showMaterials(const Array<StringID>& names);

        void collectSelectedMaterialNames(HashSet<StringID>& outNames) const;
               
    private:
        rendering::MeshPtr m_mesh;

        //--

        ui::DataInspectorPtr m_properties;
        ui::ListViewPtr m_list;
        
        MeshMaterialPreviewSettings m_settings;

        ui::Timer m_captionsRefreshTimer;

        base::RefPtr<MeshMaterialListModel> m_listModel;

        void refreshMaterialList();
        void refreshMaterialProperties();
        void updateCaptions();
    };

    //--

} // ed
