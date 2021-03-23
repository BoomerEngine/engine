/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "editor/viewport/include/uiScenePanel.h"
#include "editor/viewport/include/gizmoGroup.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--
    
// a preview panel for editor scene
class EDITOR_SCENE_EDITOR_API ScenePreviewPanel : public ui::RenderingScenePanel, public IGizmoHost
{
    RTTI_DECLARE_VIRTUAL_CLASS(ScenePreviewPanel, ui::RenderingScenePanel);

public:
    ScenePreviewPanel(ScenePreviewContainer* container);
    virtual ~ScenePreviewPanel();

    //--

    // attach edit mode to the panel, events will be routed through it
    void bindEditMode(ISceneEditMode* editMode);

    //--

    // request gizmos to be recreated for this panel
    void requestRecreateGizmo();

    //--

    virtual void configSave(const ui::ConfigBlock& block) const;
    virtual void configLoad(const ui::ConfigBlock& block);

private:
    virtual void handleFrame(rendering::FrameParams& frame) override;
    virtual void handlePointSelection(bool ctrl, bool shift, const Point& clientPosition, const Array<Selectable>& selectables) override;
    virtual void handleAreaSelection(bool ctrl, bool shift, const Rect& clientRect, const Array<Selectable>& selectables) override;
    virtual void handleContextMenu(bool ctrl, bool shift, const ui::Position& absolutePosition, const Point& clientPosition, const Selectable& objectUnderCursor, const ExactPosition* positionUnderCursor) override;
    virtual ui::InputActionPtr handleMouseClick(const ui::ElementArea& area, const InputMouseClickEvent& evt) override;
    virtual bool handleMouseMovement(const InputMouseMovementEvent& evt) override;
    virtual bool handleCursorQuery(const ui::ElementArea& area, const ui::Position& absolutePosition, CursorType& outCursorType) const override;
    virtual bool handleKeyEvent(const InputKeyEvent& evt) override;
    virtual ui::DragDropHandlerPtr handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition) override;

    virtual ui::IElement* gizmoHost_element() const override final;
    virtual bool gizmoHost_hasSelection() const override final;
    virtual const Camera& gizmoHost_camera() const override final;
    virtual Point gizmoHost_viewportSize() const override final;
    virtual GizmoReferenceSpace gizmoHost_referenceSpace() const override final;
    virtual GizmoActionContextPtr gizmoHost_startAction() const override final;

    virtual rendering::RenderingScene* scene() const override final;

    void recreateGizmo();

    SceneEditModeWeakPtr m_editMode;
    ScenePreviewContainer* m_container;

    GizmoGroupPtr m_gizmos;
};

//--

END_BOOMER_NAMESPACE_EX(ed)
