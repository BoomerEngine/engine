/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#pragma once

#include "rendering/ui/include/renderingFullScenePanel.h"

namespace ed
{
    //--
    
    // a preview panel for editor scene
    class ASSETS_SCENE_EDITOR_API ScenePreviewPanel : public ui::RenderingFullScenePanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ScenePreviewPanel, ui::RenderingFullScenePanel);

    public:
        ScenePreviewPanel(ScenePreviewContainer* container);
        virtual ~ScenePreviewPanel();

        //--

        // attach edit mode to the panel, events will be routed through it
        void bindEditMode(ISceneEditMode* editMode);

        //--

        virtual void configSave(const ui::ConfigBlock& block) const;
        virtual void configLoad(const ui::ConfigBlock& block);

    private:
        virtual void handleRender(rendering::scene::FrameParams& frame) override;
        virtual void handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual void handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual ui::InputActionPtr handleMouseClick(const ui::ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;

        SceneEditModeWeakPtr m_editMode;

        ScenePreviewContainer* m_container;
    };

    //--

} // ed