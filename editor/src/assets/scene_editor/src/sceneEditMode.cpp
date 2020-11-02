/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes #]
***/

#include "build.h"
#include "sceneEditMode.h"

namespace ed
{

    //--
     
    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(ISceneEditMode);
    RTTI_END_TYPE();

    ISceneEditMode::ISceneEditMode(ActionHistory* actionHistory)
        : m_actionHistory(actionHistory)
    {
    }

    ISceneEditMode::~ISceneEditMode()
    {
        DEBUG_CHECK(m_container == nullptr);
    }

    void ISceneEditMode::acivate(ScenePreviewContainer* container)
    {
        DEBUG_CHECK_RETURN(container != nullptr);
        DEBUG_CHECK(m_container == nullptr);
        m_container = container;
    }

    void ISceneEditMode::deactivate(ScenePreviewContainer* container)
    {
        DEBUG_CHECK_RETURN(container == m_container);
        m_container = nullptr;
    }

    ui::ElementPtr ISceneEditMode::queryUserInterface() const
    {
        return nullptr;
    }

    Array<SceneContentNodePtr> ISceneEditMode::querySelection() const
    {
        return {};
    }

    void ISceneEditMode::handleRender(ScenePreviewPanel* panel, rendering::scene::FrameParams& frame)
    {
    }

    ui::InputActionPtr ISceneEditMode::handleMouseClick(ScenePreviewPanel* panel, const input::MouseClickEvent& evt)
    {
        return nullptr;
    }

    bool ISceneEditMode::handleKeyEvent(ScenePreviewPanel* panel, const base::input::KeyEvent& evt)
    {
        return false;
    }

    void ISceneEditMode::handlePointSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
    {

    }

    void ISceneEditMode::handleAreaSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
    {

    }

    void ISceneEditMode::handleUpdate(float dt)
    {}

    void ISceneEditMode::handleTreeContextMenu(ui::MenuButtonContainer* menu, const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection)
    {}

    void ISceneEditMode::handleTreeSelectionChange(const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection)
    {}

    void ISceneEditMode::handleTreeDeleteNodes(const Array<SceneContentNodePtr>& selection)
    {}

    void ISceneEditMode::handleTreeCutNodes(const Array<SceneContentNodePtr>& selection)
    {}

    void ISceneEditMode::handleTreeCopyNodes(const Array<SceneContentNodePtr>& selection)
    {}

    void ISceneEditMode::handleTreePasteNodes(const SceneContentNodePtr& target, SceneContentNodePasteMode mode)
    {}

    //--
    
} // ed
