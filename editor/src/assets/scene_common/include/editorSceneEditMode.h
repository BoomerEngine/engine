/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "rendering/scene/include/renderingSelectable.h"

namespace ed
{
    //--

    class EditorScenePreviewContainer;
    class EditorScenePreviewPanel;

    //--

    /// editor edit mode
    class ASSETS_SCENE_COMMON_API IEditorSceneEditMode : public IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(IEditorSceneEditMode, IObject)

    public:
        IEditorSceneEditMode();
        virtual ~IEditorSceneEditMode();

        //--

        // get active editor scene container, valid only if edit mode is active
        INLINE EditorScenePreviewContainer* container() const { return m_container; }

        //--

        // activate edit mode
        void acivate(EditorScenePreviewContainer* container);

        // deactivate edit mode
        void deactivate(EditorScenePreviewContainer* container);

        //--

        // get the UI interface to show in the "Details" when this edit mode is active
        virtual ui::ElementPtr queryUserInterface() const;

        //--

        // render edit mode content, called for each panel separately
        virtual void handleRender(EditorScenePreviewPanel* panel, rendering::scene::FrameParams& frame);

        // handle custom viewport click (if no handler is returned we default to camera movement/selection)
        virtual ui::InputActionPtr handleMouseClick(EditorScenePreviewPanel* panel, const input::MouseClickEvent& evt);

        // handle custom viewport action-less keyboard event
        virtual bool handleKeyEvent(EditorScenePreviewPanel* panel, const base::input::KeyEvent& evt);

        // handle viewport selection result
        virtual void handlePointSelection(EditorScenePreviewPanel* panel, bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables);

        // handle viewport selection result
        virtual void handleAreaSelection(EditorScenePreviewPanel* panel, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables);

        // update state
        virtual void handleUpdate(float dt);

    private:
        EditorScenePreviewContainer* m_container = nullptr;
    };

    //--

} // ed
