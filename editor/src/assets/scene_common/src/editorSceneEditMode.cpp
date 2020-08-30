/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "editorSceneEditMode.h"

namespace ed
{

    //--
     
    RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IEditorSceneEditMode);
    RTTI_END_TYPE();

    IEditorSceneEditMode::IEditorSceneEditMode()
    {
    }

    IEditorSceneEditMode::~IEditorSceneEditMode()
    {
        DEBUG_CHECK(m_container == nullptr);
    }

    void IEditorSceneEditMode::acivate(EditorScenePreviewContainer* container)
    {
        DEBUG_CHECK_RETURN(container != nullptr);
        DEBUG_CHECK(m_container == nullptr);
        m_container = container;
    }

    void IEditorSceneEditMode::deactivate(EditorScenePreviewContainer* container)
    {
        DEBUG_CHECK_RETURN(container == m_container);
        m_container = nullptr;
    }

    ui::ElementPtr IEditorSceneEditMode::queryUserInterface() const
    {
        return nullptr;
    }

    void IEditorSceneEditMode::handleRender(EditorScenePreviewPanel* panel, rendering::scene::FrameParams& frame)
    {
    }

    ui::InputActionPtr IEditorSceneEditMode::handleMouseClick(EditorScenePreviewPanel* panel, const input::MouseClickEvent& evt)
    {
        return nullptr;
    }

    bool IEditorSceneEditMode::handleKeyEvent(EditorScenePreviewPanel* panel, const base::input::KeyEvent& evt)
    {
        return false;
    }

    void IEditorSceneEditMode::handlePointSelection(EditorScenePreviewPanel* panel, bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
    {

    }

    void IEditorSceneEditMode::handleAreaSelection(EditorScenePreviewPanel* panel, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
    {

    }

    void IEditorSceneEditMode::handleUpdate(float dt)
    {}

    //--
    
} // ed
