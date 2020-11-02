/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#include "build.h"
#include "sceneEditMode.h"
#include "scenePreviewPanel.h"
#include "scenePreviewContainer.h"

#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameParams.h"

namespace ed
{

    //--
     
    RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePreviewContainer);
    RTTI_END_TYPE();

    ScenePreviewContainer::ScenePreviewContainer()
    {
        createPanels();
    }

    ScenePreviewContainer::~ScenePreviewContainer()
    {
    }

    void ScenePreviewContainer::bindContent(SceneContentStructure* content, ISceneEditMode* initialEditMode /*= nullptr*/)
    {
        deactivateEditMode();

        m_content = content;

        if (initialEditMode)
            m_editMode = AddRef(initialEditMode);

        activateEditMode();
    }

    void ScenePreviewContainer::deactivateEditMode()
    {
        if (m_editMode)
        {
            m_editMode->deactivate(this);

            for (const auto& panel : m_panels)
                panel->bindEditMode(nullptr);
        }
    }

    void ScenePreviewContainer::activateEditMode()
    {
        if (m_editMode)
        {
            m_editMode->acivate(this);

            for (const auto& panel : m_panels)
                panel->bindEditMode(m_editMode);
        }
    }

    void ScenePreviewContainer::actionSwitchMode(ISceneEditMode* newMode)
    {
        DEBUG_CHECK_RETURN(m_content != nullptr);

        if (m_editMode != newMode)
        {           
            deactivateEditMode();
            m_editMode = AddRef(newMode);
            activateEditMode();

            postEvent(EVENT_EDIT_MODE_CHANGED);
        }
    }

    void ScenePreviewContainer::configSave(const ui::ConfigBlock& block) const
    {
    }

    void ScenePreviewContainer::configLoad(const ui::ConfigBlock& block)
    {
    }
    
    //--

    void ScenePreviewContainer::destroyPanels()
    {
        for (auto& panel : m_panels)
        {
            panel->bindEditMode(nullptr);
            detachChild(panel);
        }

        m_panels.clear();
    }

    void ScenePreviewContainer::createPanels()
    {
        destroyPanels();

        // TODO: different panel modes

        {
            auto panel = CreateSharedPtr<ScenePreviewPanel>(this);
            panel->expand();
            attachChild(panel);
            m_panels.pushBack(panel);
        }

        if (m_editMode)
            for (auto& panel : m_panels)
                panel->bindEditMode(m_editMode);
    }

    //--
    
} // ed
