/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#include "build.h"
#include "editorScene.h"
#include "editorSceneEditMode.h"
#include "editorScenePreviewPanel.h"
#include "editorScenePreviewContainer.h"

#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameParams.h"

namespace ed
{

    //--
     
    RTTI_BEGIN_TYPE_NATIVE_CLASS(EditorScenePreviewContainer);
    RTTI_END_TYPE();

    EditorScenePreviewContainer::EditorScenePreviewContainer(bool fullScene)
    {
        m_scene = CreateSharedPtr<EditorScene>(fullScene);
        createPanels();
    }

    EditorScenePreviewContainer::~EditorScenePreviewContainer()
    {
    }

    void EditorScenePreviewContainer::actionSwitchMode(IEditorSceneEditMode* newMode)
    {
        if (m_editMode != newMode)
        {
            if (m_editMode)
            {
                m_editMode->deactivate(this);

                for (const auto& panel : m_panels)
                    panel->bindEditMode(nullptr);
            }

            m_editMode = AddRef(newMode);

            if (m_editMode)
            {
                m_editMode->acivate(this);

                for (const auto& panel : m_panels)
                    panel->bindEditMode(m_editMode);
            }
        }
    }

    void EditorScenePreviewContainer::configSave(const ui::ConfigBlock& block) const
    {
    }

    void EditorScenePreviewContainer::configLoad(const ui::ConfigBlock& block)
    {
    }
    
    //--

    void EditorScenePreviewContainer::destroyPanels()
    {
        for (auto& panel : m_panels)
        {
            panel->bindEditMode(nullptr);
            detachChild(panel);
        }

        m_panels.clear();
    }

    void EditorScenePreviewContainer::createPanels()
    {
        destroyPanels();

        // TODO: different panel modes

        {
            auto panel = CreateSharedPtr<EditorScenePreviewPanel>(this, m_scene);
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
