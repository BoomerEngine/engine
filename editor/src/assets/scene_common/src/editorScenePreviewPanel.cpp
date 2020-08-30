/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "editorSceneEditMode.h"
#include "editorScenePreviewPanel.h"
#include "editorScene.h"

#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameParams.h"

namespace ed
{

    //--
     
    RTTI_BEGIN_TYPE_NATIVE_CLASS(EditorScenePreviewPanel);
    RTTI_END_TYPE();

    EditorScenePreviewPanel::EditorScenePreviewPanel(EditorScenePreviewContainer* container, EditorScene* scene)
        : m_container(container)
        , m_scene(AddRef(scene))
    {
        m_panelSettings.cameraForceOrbit = false;// true;
    }

    EditorScenePreviewPanel::~EditorScenePreviewPanel()
    {}

    void EditorScenePreviewPanel::configSave(const ui::ConfigBlock& block) const
    {
        TBaseClass::configSave(block);
    }

    void EditorScenePreviewPanel::configLoad(const ui::ConfigBlock& block)
    {
        TBaseClass::configLoad(block);
    }

    void EditorScenePreviewPanel::bindEditMode(IEditorSceneEditMode* editMode)
    {
        m_editMode = editMode;
    }

    void EditorScenePreviewPanel::handleRender(rendering::scene::FrameParams& frame)
    {
        TBaseClass::handleRender(frame);

        frame.scenes.scenesToDraw.clear();

        m_scene->render(frame);

        if (auto editMode = m_editMode.lock())
            editMode->handleRender(this, frame);
    }

    void EditorScenePreviewPanel::handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
    {
        if (auto editMode = m_editMode.lock())
            editMode->handlePointSelection(this, ctrl, shift, clientPosition, selectables);
    }

    void EditorScenePreviewPanel::handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
    {
        if (auto editMode = m_editMode.lock())
            editMode->handleAreaSelection(this, ctrl, shift, clientRect, selectables);
    }

    ui::InputActionPtr EditorScenePreviewPanel::handleMouseClick(const ui::ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (auto editMode = m_editMode.lock())
            if (auto action = editMode->handleMouseClick(this, evt))
                return action;

        return TBaseClass::handleMouseClick(area, evt);
    }

    bool EditorScenePreviewPanel::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (auto editMode = m_editMode.lock())
            if (editMode->handleKeyEvent(this, evt))
                return true;

        return TBaseClass::handleKeyEvent(evt);
    }

    //--
    
} // ed
