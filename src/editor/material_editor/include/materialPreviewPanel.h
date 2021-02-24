/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "rendering/ui_viewport/include/renderingScenePanel.h"

BEGIN_BOOMER_NAMESPACE(ed)

//--

enum class MaterialPreviewShape
{
    Box,
    Sphere,
    Cylinder,
    Plane,
    Custom,
};

struct EDITOR_MATERIAL_EDITOR_API MaterialPreviewPanelSettings
{
    MaterialPreviewShape shape = MaterialPreviewShape::Box;
    int mode = 0;
    base::res::Ref<rendering::Mesh> customMesh;

    MaterialPreviewPanelSettings();
};

//--

// a preview panel for the material
class EDITOR_MATERIAL_EDITOR_API MaterialPreviewPanel : public ui::RenderingSimpleScenePanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(MaterialPreviewPanel, ui::RenderingSimpleScenePanel);

public:
    MaterialPreviewPanel();
    virtual ~MaterialPreviewPanel();

    inline const MaterialPreviewPanelSettings& previewSettings() const { return m_previewSettings; }

    void previewSettings(const MaterialPreviewPanelSettings& settings);
    void previewShape(MaterialPreviewShape shape);

    void bindMaterial(const rendering::IMaterial* material);

    virtual void buildShapePopup(ui::MenuButtonContainer* menu);

private:
    MaterialPreviewPanelSettings m_previewSettings;

    rendering::scene::ObjectProxyPtr m_previewProxy;
    rendering::MaterialPtr m_material;

    void destroyVisualization();
    void createVisualization();

    void createToolbarItems();

    virtual bool computeContentBounds(base::Box& outBox) const override;
    virtual void handleRender(rendering::scene::FrameParams& frame) override;

    virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;
    virtual void handleDragDropGenericCompletion(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;
};
    
//--

END_BOOMER_NAMESPACE(ed)