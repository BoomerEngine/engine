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
    
    // a preview panel for editor scene
    class ASSETS_SCENE_COMMON_API EditorScenePreviewPanel : public ui::RenderingFullScenePanel
    {
        RTTI_DECLARE_VIRTUAL_CLASS(EditorScenePreviewPanel, ui::RenderingFullScenePanel);

    public:
        EditorScenePreviewPanel(EditorScenePreviewContainer* container, EditorScene* scene);
        virtual ~EditorScenePreviewPanel();

        //--

        // attach edit mode to the panel, events will be routed through it
        void bindEditMode(IEditorSceneEditMode* editMode);

        //--

        virtual void configSave(const ui::ConfigBlock& block) const;
        virtual void configLoad(const ui::ConfigBlock& block);

    private:
        virtual void handleRender(rendering::scene::FrameParams& frame) override;
        virtual void handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual void handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables) override;
        virtual ui::InputActionPtr handleMouseClick(const ui::ElementArea& area, const base::input::MouseClickEvent& evt) override;
        virtual bool handleKeyEvent(const base::input::KeyEvent& evt) override;

        EditorSceneEditModeWeakPtr m_editMode;

        EditorScenePreviewContainer* m_container;
        EditorScenePtr m_scene;
    };

    //--

} // ed