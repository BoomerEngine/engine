/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: aspects #]
***/

#pragma once

#include "base/ui/include/uiSimpleListModel.h"
#include "base/ui/include/uiAbstractItemModel.h"
#include "base/ui/include/uiDragDrop.h"
#include "assets/mesh_loader/include/rendernigMeshMaterialConfig.h"

namespace ed
{
    //--

    class MeshPreviewPanelWithToolbar;
    class MeshEditorMaterialInstance;

    //--

    /// helper object for editable mesh material
    class MeshMaterialParameters : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshMaterialParameters, base::IObject);

    public:
        MeshMaterialParameters(rendering::MeshMaterialConfig* manifest, base::StringID name, const rendering::MaterialRef& baseMaterial);

        INLINE base::StringID name() const { return m_name; }

        INLINE const base::StringBuf& displayString() const { return m_displayString; }

        INLINE const rendering::MaterialRef& baseParams() const { return m_baseMaterial; }

        INLINE const base::RefPtr<MeshEditorMaterialInstance>& previewParams() const { return m_previewParams; }

        bool updateDisplayString();

        bool baseMaterial(const rendering::MaterialRef& material);

    private:
        rendering::MeshMaterialConfig* m_manifest = nullptr;

        base::StringID m_name;
        base::StringBuf m_displayString;

        rendering::MaterialRef m_baseMaterial;
        base::RefPtr<MeshEditorMaterialInstance> m_previewParams;
    };

    //--

    /// list model for material list
    class MeshMaterialListModel : public ui::SimpleTypedListModel<base::RefPtr<MeshMaterialParameters>>
    {
    public:
        MeshMaterialListModel(rendering::MeshMaterialConfig* manifest);

        virtual bool compare(const base::RefPtr<MeshMaterialParameters>& a, const base::RefPtr<MeshMaterialParameters>& b, int colIndex) const override final;
        virtual bool filter(const base::RefPtr<MeshMaterialParameters>& data, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf content(const base::RefPtr<MeshMaterialParameters>& data, int colIndex = 0) const override final;

        virtual ui::DragDropHandlerPtr handleDragDropData(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data, const ui::Position& pos) override final;
        virtual bool handleDragDropCompletion(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data) override final;

    private:
        rendering::MeshMaterialConfig* m_manifest = nullptr;
    };

    //--

    /// editor aspect for displaying the mesh packing parameters
    class MeshMaterialsPanel : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshMaterialsPanel, ui::IElement);

    public:
        MeshMaterialsPanel(MeshPreviewPanel* previewPanel, base::ActionHistory* actionHistory);

        void bindResource(const rendering::MeshPtr& mesh);

    private:
        MeshPreviewPanel* m_previewPanel = nullptr;
        rendering::MeshPtr m_mesh;

        //--

        ui::DataInspectorPtr m_properties;
        ui::ListViewPtr m_list;

        ui::Timer m_captionsRefreshTimer;

        base::RefPtr<MeshMaterialListModel> m_listModel;

        void refreshMaterialList();
        void refreshMaterialProperties();
        void updateCaptions();
    };

    //--

} // ed
