/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "rendering/ui_viewport/include/renderingScenePanel.h"
#include "assets/gizmos/include/gizmoGroup.h"

namespace ed
{
    //--
    
    // a preview panel for editor scene
    class ASSETS_SCENE_EDITOR_API ScenePreviewPanel : public ui::RenderingScenePanel, public IGizmoHost
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
        virtual void handleRender(rendering::scene::FrameParams& frame) override;
        virtual void handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual void handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual void handleContextMenu(bool ctrl, bool shift, const ui::Position& absolutePosition, const base::Point& clientPosition, const rendering::scene::Selectable& objectUnderCursor, const base::AbsolutePosition* positionUnderCursor) override;
        virtual ui::InputActionPtr handleMouseClick(const ui::ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual bool handleMouseMovement(const base::input::MouseMovementEvent& evt) override;
        virtual bool handleCursorQuery(const ui::ElementArea& area, const ui::Position& absolutePosition, base::input::CursorType& outCursorType) const override;
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;

        virtual ui::IElement* gizmoHost_element() const override final;
        virtual bool gizmoHost_hasSelection() const override final;
        virtual const rendering::scene::Camera& gizmoHost_camera() const override final;
        virtual base::Point gizmoHost_viewportSize() const override final;
        virtual GizmoReferenceSpace gizmoHost_referenceSpace() const override final;
        virtual GizmoActionContextPtr gizmoHost_startAction() const override final;

        void recreateGizmo();

        SceneEditModeWeakPtr m_editMode;
        ScenePreviewContainer* m_container;

        GizmoGroupPtr m_gizmos;
    };

    //--

} // ed