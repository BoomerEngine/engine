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

#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameParams.h"

#include "base/world/include/world.h"

namespace ed
{

    //--
     
    RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePreviewPanel);
    RTTI_END_TYPE();

    ScenePreviewPanel::ScenePreviewPanel(ScenePreviewContainer* container)
        : m_container(container)
    {
        m_panelSettings.cameraForceOrbit = false;// true;
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
    }

    void ScenePreviewPanel::handleRender(rendering::scene::FrameParams& frame)
    {
        TBaseClass::handleRender(frame);

        frame.scenes.scenesToDraw.clear();

        if (m_container)
        {
            m_container->world()->render(frame);
            m_container->content()->handleDebugRender(frame);
        }

        if (auto editMode = m_editMode.lock())
            editMode->handleRender(this, frame);
    }

    void ScenePreviewPanel::handlePointSelection(bool ctrl, bool shift, const base::Point& clientPosition, const base::Array<rendering::scene::Selectable>& selectables)
    {
        if (auto editMode = m_editMode.lock())
            editMode->handlePointSelection(this, ctrl, shift, clientPosition, selectables);
    }

    void ScenePreviewPanel::handleAreaSelection(bool ctrl, bool shift, const base::Rect& clientRect, const base::Array<rendering::scene::Selectable>& selectables)
    {
        if (auto editMode = m_editMode.lock())
            editMode->handleAreaSelection(this, ctrl, shift, clientRect, selectables);
    }

    ui::InputActionPtr ScenePreviewPanel::handleMouseClick(const ui::ElementArea& area, const base::input::MouseClickEvent& evt)
    {
        if (auto editMode = m_editMode.lock())
            if (auto action = editMode->handleMouseClick(this, evt))
                return action;

        return TBaseClass::handleMouseClick(area, evt);
    }

    bool ScenePreviewPanel::handleKeyEvent(const base::input::KeyEvent& evt)
    {
        if (auto editMode = m_editMode.lock())
            if (editMode->handleKeyEvent(this, evt))
                return true;

        return TBaseClass::handleKeyEvent(evt);
    }

    //--
    
} // ed
