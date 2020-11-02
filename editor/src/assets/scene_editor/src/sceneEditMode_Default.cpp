/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\modes\default #]
***/

#include "build.h"
#include "sceneEditMode_Default.h"
#include "sceneEditMode_Default_UI.h"
#include "sceneContentNodes.h"
#include "sceneContentStructure.h"
#include "scenePreviewContainer.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/object/include/actionHistory.h"


namespace ed
{

    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(SceneEditMode_Default);
    RTTI_END_TYPE();
     
    SceneEditMode_Default::SceneEditMode_Default(ActionHistory* actionHistory)
        : ISceneEditMode(actionHistory)
    {
        m_panel = CreateSharedPtr<SceneDefaultPropertyInspectorPanel>(this);
    }

    SceneEditMode_Default::~SceneEditMode_Default()
    {}

    void SceneEditMode_Default::activeNode(SceneContentNode* node)
    {
        if (m_activeNode != node)
        {
            if (auto old = m_activeNode.lock())
                old->visualFlag(SceneContentNodeVisualBit::ActiveNode, false);

            m_activeNode = node;

            if (node)
                node->visualFlag(SceneContentNodeVisualBit::ActiveNode, true);
        }
    }

    void SceneEditMode_Default::handleSelectionChanged()
    {
        m_panel->bind(m_selection.keys());

        container()->postEvent(EVENT_EDIT_MODE_SELECTION_CHANGED);
    }

    void SceneEditMode_Default::changeSelection(const Array<SceneContentNodePtr>& selection)
    {
        HashSet<SceneContentNodePtr> newSelectionSet;
        newSelectionSet.reserve(selection.size());

        for (const auto& ptr : selection)
            newSelectionSet.insert(ptr);

        for (const auto& cur : m_selection)
            if (!newSelectionSet.contains(cur))
                cur->visualFlag(SceneContentNodeVisualBit::SelectedNode, false);

        for (const auto& cur : newSelectionSet)
            if (!m_selection.contains(cur))
                cur->visualFlag(SceneContentNodeVisualBit::SelectedNode, true);

        m_selection = std::move(newSelectionSet);

        handleSelectionChanged();
    }

    void SceneEditMode_Default::reset()
    {
        m_selection.reset();
    }

    ui::ElementPtr SceneEditMode_Default::queryUserInterface() const
    {
        return m_panel;
    }

    Array<SceneContentNodePtr> SceneEditMode_Default::querySelection() const
    {
        return m_selection.keys();
    }

    void SceneEditMode_Default::handleRender(ScenePreviewPanel* panel, rendering::scene::FrameParams& frame)
    {
        TBaseClass::handleRender(panel, frame);
    }

    ui::InputActionPtr SceneEditMode_Default::handleMouseClick(ScenePreviewPanel* panel, const input::MouseClickEvent& evt)
    {
        return nullptr;
    }

    bool SceneEditMode_Default::handleKeyEvent(ScenePreviewPanel* panel, const base::input::KeyEvent& evt)
    {
        return false;
    }

    void SceneEditMode_Default::handlePointSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
    {
    }

    void SceneEditMode_Default::handleAreaSelection(ScenePreviewPanel* panel, bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
    {
    }

    void SceneEditMode_Default::handleUpdate(float dt)
    {

    }

    void SceneEditMode_Default::handleTreeSelectionChange(const SceneContentNodePtr& context, const Array<SceneContentNodePtr>& selection)
    {
        changeSelection(selection);
    }

    //--
    
} // ed
