/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#pragma once

#include "rendering/scene/include/renderingSelectable.h"
#include "rendering/scene/include/renderingFrame.h"

#include "sceneEditorStructure.h"

namespace ed
{
    namespace world
    {

        /// scene edit mode
        class EDITOR_SCENE_MAIN_API ISceneEditMode : public base::NoCopy
        {
            RTTI_DECLARE_VIRTUAL_ROOT_CLASS(ISceneEditMode);

        public:
            ISceneEditMode();
            virtual ~ISceneEditMode();

            //--

            /// get the editor this edit mode is assigned to
            INLINE SceneEditorTab* editor() const { return m_editor; }

            /// get edit mode caption
            virtual base::StringBuf name() const = 0;

            /// get edit mode icon (if specified than edit mode is added to the toolbar)
            virtual base::image::ImagePtr icon() const = 0;

            /// get UI to display for this edit mode while it's active (a side panel)
            virtual ui::ElementPtr uI() const;

            //--

            /// initialize edit mode for given scene editor, can be refused if for some reason given scene content is not supported
            virtual bool initialize(SceneEditorTab* owner);

            /// activate the mode, always called due to action of the user, can be refused
            virtual bool activate(const base::Array<ContentNodePtr>& activeSceneSelection) = 0;

            /// deactivate the edit mode
            virtual void deactive();

            /// active scene selection was changed (may be due to undo/redo or other user action, adjust edit mode)
            virtual void bind(const base::Array<ContentNodePtr>& activeSceneSelection);

            /// handle editor configuration update (usually gizmo/grid/selection settings, etc)
            virtual void configure(base::ClassType dataType, const void* data, bool active);

            //--

            /// handle selection of objects from the viewport
            virtual void handleViewportSelection(const SceneRenderingPanel* viewport, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables);

            /// handle a context menu request
            virtual bool handleContextMenu(const SceneRenderingPanel* viewport, const ui::Position& clickPosition, const base::Array<rendering::scene::Selectable>& selectables, const base::Vector3* worldSpacePosition);

            /// handle action click
            virtual ui::InputActionPtr handleMouseClick(const SceneRenderingPanel* viewport, const base::input::MouseClickEvent& evt);

            /// handle out of bounds key event
            virtual bool handleKeyEvent(const SceneRenderingPanel* viewport, const base::input::KeyEvent &evt);

            /// handle general mouse movement in non-captured mode
            virtual bool handleMouseMovement(const SceneRenderingPanel* viewport, const base::input::MouseMovementEvent& evt);

            /// handle mouse wheel
            virtual bool handleMouseWheel(const SceneRenderingPanel* viewport, const base::input::MouseMovementEvent& evt, float delta);

            /// handle general tick, called only if edit mode is active
            virtual void handleTick(float dt);

            /// handle rendering
            virtual void handleRendering(const SceneRenderingPanel* viewport, rendering::scene::FrameInfo& frame);

            /// handle drag&drop
            virtual ui::DragDropHandlerPtr handleDragDrop(const SceneRenderingPanel* viewport, const ui::DragDropDataPtr& data, const ui::Position& entryPosition);

            //---

            /// can we copy whatever is currently active ?
            virtual bool canCopy() const;

            /// can we cut whatever is currently active ?
            virtual bool canCut() const;

            /// can we paste content ?
            virtual bool canPaste() const;

            /// can we delete whatever is currently active ?
            virtual bool canDelete() const;

            /// handle copy operation
            virtual void handleCopy();

            /// handle cut operation
            virtual void handleCut();

            /// handle cut operation
            virtual void handlePaste();

            /// handle cut operation
            virtual void handleDelete();

            //---

        private:
            SceneEditorTab* m_editor; // active document we are attached to
        };

    } // mesh
} // ed