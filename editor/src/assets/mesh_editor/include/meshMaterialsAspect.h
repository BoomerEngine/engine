/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: aspects #]
***/

#pragma once

#include "base/editor/include/singleResourceEditor.h"
#include "base/ui/include/uiSimpleListModel.h"
#include "base/ui/include/uiAbstractItemModel.h"
#include "base/ui/include/uiDragDrop.h"
#include "assets/mesh_loader/include/rendernigMeshMaterialManifest.h"

namespace ed
{
    //--

    class MeshPreviewPanelWithToolbar;

    //--

    /// helper object for editable mesh material
    class MeshMaterialParameters : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshMaterialParameters, base::IObject);

    public:
        MeshMaterialParameters(rendering::MeshMaterialBindingManifest* manifest, base::StringID name, const rendering::MaterialInstancePtr& meshBaseParams);

        INLINE base::StringID name() const { return m_name; }

        INLINE const base::StringBuf& displayString() const { return m_displayString; }

        INLINE const rendering::MaterialInstancePtr& baseParams() const { return m_baseParams; }

        INLINE const rendering::MaterialInstancePtr& previewParams() const { return m_previewParams; }

        bool updateDisplayString();

        bool baseMaterial(const rendering::MaterialRef& material);

    private:
        rendering::MeshMaterialBindingManifest* m_manifest = nullptr;

        base::StringID m_name;
        base::StringBuf m_displayString;

        rendering::MaterialInstancePtr m_baseParams;
        rendering::MaterialInstancePtr m_previewParams;
    };

    //--

    /// list model for material list
    class MeshMaterialListModel : public ui::SimpleTypedListModel<base::RefPtr<MeshMaterialParameters>>
    {
    public:
        MeshMaterialListModel(rendering::MeshMaterialBindingManifest* manifest);

        virtual bool compare(const base::RefPtr<MeshMaterialParameters>& a, const base::RefPtr<MeshMaterialParameters>& b, int colIndex) const override final;
        virtual bool filter(const base::RefPtr<MeshMaterialParameters>& data, const ui::SearchPattern& filter, int colIndex = 0) const override final;
        virtual base::StringBuf content(const base::RefPtr<MeshMaterialParameters>& data, int colIndex = 0) const override final;

        virtual ui::DragDropHandlerPtr handleDragDropData(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data, const ui::Position& pos) override final;
        virtual bool handleDragDropCompletion(ui::AbstractItemView* view, const ui::ModelIndex& item, const ui::DragDropDataPtr& data) override final;

    private:
        rendering::MeshMaterialBindingManifest* m_manifest = nullptr;
    };

    //--

    /// editor aspect for displaying the mesh packing parameters
    class MeshMaterialBindingAspect : public SingleResourceEditorManifestAspect
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MeshMaterialBindingAspect, SingleResourceEditorManifestAspect);

    public:
        MeshMaterialBindingAspect();

        virtual bool initialize(SingleResourceEditor* editor) override;

    private:
        MeshEditor* m_meshEditor = nullptr;
        MeshPreviewPanelWithToolbar* m_previewPanel = nullptr;

        base::RefPtr<rendering::MeshMaterialBindingManifest> m_materialManifest;

        ui::DataInspectorPtr m_properties;
        ui::ListViewPtr m_list;
        base::UniquePtr<ui::Timer> m_timer;

        base::RefPtr<MeshMaterialListModel> m_listModel;

        void refreshMaterialList();
        void refreshMaterialProperties();
        void updateCaptions();

        virtual void previewResourceChanged() override;
    };

    //--

} // ed
