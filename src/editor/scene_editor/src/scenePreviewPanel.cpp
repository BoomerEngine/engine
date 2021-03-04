/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#include "build.h"
#include "sceneEditMode.h"
#include "sceneContentStructure.h"
#include "scenePreviewContainer.h"
#include "scenePreviewPanel.h"

#include "engine/rendering/include/scene.h"
#include "engine/rendering/include/debug.h"
#include "engine/rendering/include/params.h"

#include "engine/world/include/world.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--
     
RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePreviewPanel);
RTTI_END_TYPE();

ScenePreviewPanel::ScenePreviewPanel(ScenePreviewContainer* container)
    : m_container(container)
{
}

ScenePreviewPanel::~ScenePreviewPanel()
{}

void ScenePreviewPanel::configSave(const ui::ConfigBlock& block) const
{
    TBaseClass::configSave(block);
}

void ScenePreviewPanel::configLoad(const ui::ConfigBlock& block)
{
    TBaseClass::configLoad(block);
}

void ScenePreviewPanel::bindEditMode(ISceneEditMode* editMode)
{
    m_editMode = editMode;
    recreateGizmo();
}

void ScenePreviewPanel::requestRecreateGizmo()
{
    recreateGizmo();
}

rendering::Scene* ScenePreviewPanel::scene() const
{
    return nullptr; // TODO
}

void ScenePreviewPanel::handleRender(rendering::FrameParams& frame)
{
    TBaseClass::handleRender(frame);

    if (m_container)
    {
        m_container->world()->render(frame);
        m_container->content()->handleDebugRender(frame);
    }

    if (auto editMode = m_editMode.lock())
        editMode->handleRender(this, frame);

    if (m_gizmos)
        m_gizmos->render(frame);
}

ui::DragDropHandlerPtr ScenePreviewPanel::handleDragDrop(const ui::DragDropDataPtr& data, const ui::Position& entryPosition)
{
    if (auto editMode = m_editMode.lock())
    {
        const auto clientPoint = entryPosition - cachedDrawArea().absolutePosition();
        return editMode->handleDragDrop(this, data, entryPosition, clientPoint);
    }

    return nullptr;
}

void ScenePreviewPanel::handlePointSelection(bool ctrl, bool shift, const Point& clientPosition, const Array<Selectable>& selectables)
{
    if (auto editMode = m_editMode.lock())
        editMode->handlePointSelection(this, ctrl, shift, clientPosition, selectables);
}

void ScenePreviewPanel::handleAreaSelection(bool ctrl, bool shift, const Rect& clientRect, const Array<Selectable>& selectables)
{
    if (auto editMode = m_editMode.lock())
        editMode->handleAreaSelection(this, ctrl, shift, clientRect, selectables);
}

void ScenePreviewPanel::handleContextMenu(bool ctrl, bool shift, const ui::Position& absolutePosition, const Point& clientPosition, const Selectable& objectUnderCursor, const AbsolutePosition* positionUnderCursor)
{
    if (auto editMode = m_editMode.lock())
        editMode->handleContextMenu(this, ctrl, shift, absolutePosition, clientPosition, objectUnderCursor, positionUnderCursor);
}

ui::InputActionPtr ScenePreviewPanel::handleMouseClick(const ui::ElementArea& area, const input::MouseClickEvent& evt)
{
    if (evt.leftClicked() && m_gizmos)
    {
        const auto clientPoint = evt.absolutePosition() - area.absolutePosition();
        if (m_gizmos->updateHover(clientPoint))
            if (auto action = m_gizmos->activate())
                return action;
    }

    if (auto editMode = m_editMode.lock())
        if (auto action = editMode->handleMouseClick(this, evt))
            return action;

    return TBaseClass::handleMouseClick(area, evt);
}

bool ScenePreviewPanel::handleMouseMovement(const input::MouseMovementEvent& evt)
{
    if (m_gizmos)
    {
        const auto clientPoint = evt.absolutePosition() - cachedDrawArea().absolutePosition();
        m_gizmos->updateHover(clientPoint);
    }

    return TBaseClass::handleMouseMovement(evt);
}

bool ScenePreviewPanel::handleCursorQuery(const ui::ElementArea& area, const ui::Position& absolutePosition, input::CursorType& outCursorType) const
{
    if (m_gizmos)
    {
        const auto clientPoint = absolutePosition - area.absolutePosition();
        if (m_gizmos->updateHover(clientPoint))
            if (m_gizmos->updateCursor(outCursorType))
                return true;
    }

    return TBaseClass::handleCursorQuery(area, absolutePosition, outCursorType);
}

bool ScenePreviewPanel::handleKeyEvent(const input::KeyEvent& evt)
{
    if (auto editMode = m_editMode.lock())
        if (editMode->handleKeyEvent(this, evt))
            return true;

    return TBaseClass::handleKeyEvent(evt);
}

//--

ui::IElement* ScenePreviewPanel::gizmoHost_element() const
{
    return const_cast<ScenePreviewPanel*>(this);
}

bool ScenePreviewPanel::gizmoHost_hasSelection() const
{
    if (auto editMode = m_editMode.lock())
        return editMode && editMode->hasSelection();
    return false;
}

const Camera& ScenePreviewPanel::gizmoHost_camera() const
{
    return cachedCamera();
}

Point ScenePreviewPanel::gizmoHost_viewportSize() const
{
    return cachedDrawArea().size();
}

GizmoReferenceSpace ScenePreviewPanel::gizmoHost_referenceSpace() const
{
    if (auto editMode = m_editMode.lock())
        return editMode->calculateGizmoReferenceSpace();
    return GizmoReferenceSpace();
}

GizmoActionContextPtr ScenePreviewPanel::gizmoHost_startAction() const
{
    if (auto editMode = m_editMode.lock())
        return editMode->createGizmoAction(m_container, this);
    return nullptr;
}

//--

void ScenePreviewPanel::recreateGizmo()
{
    m_gizmos.reset();

    if (auto editMode = m_editMode.lock())
        m_gizmos = editMode->configurePanelGizmos(m_container, this);
}

//--

END_BOOMER_NAMESPACE_EX(ed)
