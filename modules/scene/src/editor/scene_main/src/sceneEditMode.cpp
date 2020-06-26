/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: world #]
***/

#include "build.h"
#include "sceneEditMode.h"
#include "sceneEditor.h"

namespace ed
{
    namespace world
    {

        RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISceneEditMode);
        RTTI_END_TYPE();

        //--

        ISceneEditMode::ISceneEditMode()
            : m_editor(nullptr)
        {}

        ISceneEditMode::~ISceneEditMode()
        {}

        ui::ElementPtr ISceneEditMode::uI() const
        {
            return nullptr;
        }

        bool ISceneEditMode::initialize(SceneEditorTab* owner)
        {
            m_editor = owner;
            return true;
        }

        void ISceneEditMode::deactive()
        {
        }

        void ISceneEditMode::bind(const base::Array<ContentNodePtr>& activeSceneSelection)
        {
        }

        void ISceneEditMode::configure(base::ClassType dataType, const void* data, bool active)
        {
        }

        //--

        void ISceneEditMode::handleViewportSelection(const SceneRenderingPanel* viewport, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
        {
        }

        bool ISceneEditMode::handleContextMenu(const SceneRenderingPanel* viewport, const ui::Position& clickPosition, const base::Array<rendering::scene::Selectable>& selectables, const base::Vector3* worldSpacePosition)
        {
            return false;
        }

        ui::InputActionPtr ISceneEditMode::handleMouseClick(const SceneRenderingPanel* viewport, const base::input::MouseClickEvent& evt)
        {
            return nullptr;
        }

        bool ISceneEditMode::handleKeyEvent(const SceneRenderingPanel* viewport, const base::input::KeyEvent& evt)
        {
            return false;
        }

        bool ISceneEditMode::handleMouseMovement(const SceneRenderingPanel* viewport, const base::input::MouseMovementEvent& evt)
        {
            return false;
        }

        bool ISceneEditMode::handleMouseWheel(const SceneRenderingPanel* viewport, const base::input::MouseMovementEvent& evt, float delta)
        {
            return false;
        }

        void ISceneEditMode::handleTick(float dt)
        {
            
        }

        void ISceneEditMode::handleRendering(const SceneRenderingPanel* viewport, rendering::scene::FrameInfo& frame)
        {
            
        }

        ui::DragDropHandlerPtr ISceneEditMode::handleDragDrop(const SceneRenderingPanel* viewport, const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
        {
            return nullptr;
        }

        //---

        bool ISceneEditMode::canCopy() const
        {
            return false;
        }

        bool ISceneEditMode::canCut() const
        {
            return false;
        }

        bool ISceneEditMode::canPaste() const
        {
            return false;
        }

        bool ISceneEditMode::canDelete() const
        {
            return false;
        }

        void ISceneEditMode::handleCopy()
        {

        }

        void ISceneEditMode::handleCut()
        {

        }

        void ISceneEditMode::handlePaste()
        {

        }

        void ISceneEditMode::handleDelete()
        {

        }

        //---

    } // world
} // ed
