/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "rendering/ui/include/renderingFullScenePanel.h"

namespace ed
{
    //--

    enum class MaterialPreviewShape
    {
        Box,
        Sphere,
        Cylinder,
        Plane,
        Custom,
    };

    struct ASSETS_MATERIAL_EDITOR_API MaterialPreviewPanelSettings
    {
        MaterialPreviewShape shape = MaterialPreviewShape::Box;
        int mode = 0;
        base::res::Ref<rendering::Mesh> customMesh;

        MaterialPreviewPanelSettings();
    };

    //--

    // a preview panel for the material
    class ASSETS_MATERIAL_EDITOR_API MaterialPreviewPanel : public ui::RenderingFullScenePanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialPreviewPanel, ui::RenderingFullScenePanel);

    public:
        MaterialPreviewPanel();
        virtual ~MaterialPreviewPanel();

        inline const MaterialPreviewPanelSettings& previewSettings() const { return m_previewSettings; }

        void previewSettings(const MaterialPreviewPanelSettings& settings);

        void bindMaterial(const rendering::MaterialRef& material);

    private:
        MaterialPreviewPanelSettings m_previewSettings;

        rendering::scene::ProxyHandle m_previewProxy;
        rendering::MaterialRef m_material;

        void destroyVisualization();
        void createVisualization();

        virtual bool computeContentBounds(base::Box& outBox) const override;
        virtual void handleRender(rendering::scene::FrameParams& frame) override;

        virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;
        virtual void handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;

        virtual void onPropertyChanged(base::StringView<char> path) override;
    };

    //--

    // preview panel with toolbar
    class ASSETS_MATERIAL_EDITOR_API MaterialPreviewPanelWithToolbar : public ui::IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialPreviewPanelWithToolbar, ui::IElement);

    public:
        MaterialPreviewPanelWithToolbar();
        virtual ~MaterialPreviewPanelWithToolbar();

        const MaterialPreviewPanelSettings& previewSettings() const;
        void previewSettings(const MaterialPreviewPanelSettings& settings);

        void bindMaterial(const rendering::MaterialRef& material);

    private:
        ui::ToolBarPtr m_toolbar;
        base::RefPtr<MaterialPreviewPanel> m_panel;

        void changePreviewShape(MaterialPreviewShape shape);
    };

    //--

} // ed