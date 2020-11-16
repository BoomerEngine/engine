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
#include "scenePreviewStreaming.h"
#include "sceneContentStructure.h"

#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingSceneProxyDesc.h"
#include "rendering/scene/include/renderingFrameDebug.h"
#include "rendering/scene/include/renderingFrameParams.h"

#include "base/ui/include/uiToolBar.h"
#include "base/world/include/world.h"
#include "sceneContentNodes.h"

namespace ed
{
    //--

    SceneGridSettings::SceneGridSettings()
    {}

    //--

    SceneGizmoSettings::SceneGizmoSettings()
    {}

    //--

    SceneSelectionSettings::SceneSelectionSettings()
    {}

    //--

    RTTI_BEGIN_TYPE_ENUM(SceneGizmoMode);
    RTTI_ENUM_OPTION(Translation);
    RTTI_ENUM_OPTION(Rotation);
    RTTI_ENUM_OPTION(Scale);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(GizmoSpace);
    RTTI_ENUM_OPTION(World);
    RTTI_ENUM_OPTION(Local);
    RTTI_ENUM_OPTION(Parent);
    RTTI_ENUM_OPTION(View);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_ENUM(SceneGizmoTarget);
    RTTI_ENUM_OPTION(WholeHierarchy);
    RTTI_ENUM_OPTION(SelectionOnly);
    RTTI_END_TYPE();

    //--
     
    RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePreviewContainer);
    RTTI_END_TYPE();

    ScenePreviewContainer::ScenePreviewContainer(SceneContentStructure* content, ISceneEditMode* initialEditMode)
        : m_content(content)
    {
        createWorld();
        createPanels();

        m_editMode = AddRef(initialEditMode);
        activateEditMode();
    }

    ScenePreviewContainer::~ScenePreviewContainer()
    {
        deactivateEditMode();

        m_visualization->clearAllProxies();
        m_visualization.reset();
    }
    
    void ScenePreviewContainer::gridSettings(const SceneGridSettings& settings)
    {
        m_gridSettings = settings;
        recreatePanelBottomToolbars();
    }

    void ScenePreviewContainer::gizmoSettings(const SceneGizmoSettings& settings)
    {
        bool needsGizmoRecreation = false;
        if (settings.mode != m_gizmoSettings.mode)
            needsGizmoRecreation = true;
        if (settings.enableX != m_gizmoSettings.enableX || settings.enableY != m_gizmoSettings.enableY || settings.enableZ != m_gizmoSettings.enableZ)
            needsGizmoRecreation = true;

        m_gizmoSettings = settings;
        recreatePanelBottomToolbars();

        if (needsGizmoRecreation)
            requestRecreatePanelGizmos();
    }

    static bool IsDevivedNode(const SceneContentNode* node)
    {
        if (node && node->type() == SceneContentNodeType::Entity)
        {
            const auto* entityNode = static_cast<const SceneContentEntityNode*>(node);
            if (entityNode->baseData())
                return true;
        }

        return false;
    }

    SceneContentNodePtr ScenePreviewContainer::resolveSelectable(const rendering::scene::Selectable& selectable, bool raw /*= false*/) const
    {
        auto node = m_visualization->resolveSelectable(selectable);

        if (node && !raw)
        {
            if (node->type() == SceneContentNodeType::Component && !m_selectionSettings.exploreComponents)
                node = AddRef(node->parent());

            if (!m_selectionSettings.explorePrefabs)
                while (IsDevivedNode(node))
                    node = AddRef(node->parent());
        }

        return node;
    }

    void ScenePreviewContainer::selectionSettings(const SceneSelectionSettings& settings)
    {
        m_selectionSettings = settings;
        recreatePanelBottomToolbars();
    }

    void ScenePreviewContainer::deactivateEditMode()
    {
        if (m_editMode)
        {
            m_editMode->deactivate(this);

            for (const auto& panel : m_panels)
            {
                panel->bindEditMode(nullptr);

                if (const auto& toolbar = panel->bottomToolbar())
                    toolbar->removeAllChildren();
            }
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

        recreatePanelBottomToolbars();
    }

    void ScenePreviewContainer::actionSwitchMode(ISceneEditMode* newMode)
    {
        DEBUG_CHECK_RETURN(m_content != nullptr);

        if (m_editMode != newMode)
        {           
            deactivateEditMode();
            m_editMode = AddRef(newMode);
            activateEditMode();

            recreatePanelBottomToolbars();

            call(EVENT_EDIT_MODE_CHANGED);
        }
    }

    void ScenePreviewContainer::configSave(const ui::ConfigBlock& block) const
    {
    }

    void ScenePreviewContainer::configLoad(const ui::ConfigBlock& block)
    {
    }
    
    //--
    
    void ScenePreviewContainer::update()
    {
        syncNodeState();
        updateWorld();
    }

    void ScenePreviewContainer::syncNodeState()
    {
        PC_SCOPE_LVL0(SyncNodeState);

        {
            Array<SceneContentNodePtr> addedNodes, removedNodes;
            m_content->syncNodeChanges(addedNodes, removedNodes);

            {
                PC_SCOPE_LVL0(CreateProxies);
                for (const auto& node : addedNodes)
                    m_visualization->createProxy(node);
            }

            {
                PC_SCOPE_LVL0(DestroyProxies);
                for (const auto& node : removedNodes)
                    m_visualization->removeProxy(node);
            }
        }

        {
            PC_SCOPE_LVL0(UpdateProxies);
            m_content->visitDirtyNodes([this](const SceneContentNode* node, SceneContentNodeDirtyFlags& flags)
                {
                    m_visualization->updateProxy(node, flags);
                });
        }

        m_visualization->updateAllProxies();
    }

    void ScenePreviewContainer::updateWorld()
    {
        auto realWorldTime = m_lastWorldTick.timeTillNow().toSeconds();
        m_lastWorldTick.resetToNow();

        auto dt = std::clamp<float>(realWorldTime * m_worldTickRatio, 0.0001f, 0.1f);
        m_world->update(dt);
    }

    void ScenePreviewContainer::createWorld()
    {
        m_world = CreateSharedPtr<world::World>();
        m_lastWorldTick.resetToNow();

        m_visualization = CreateSharedPtr<SceneNodeVisualizationHandler>(m_world);
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

        for (auto& panel : m_panels)
        {
            if (m_editMode)
                panel->bindEditMode(m_editMode);

            if (const auto& toolbar = panel->bottomToolbar())
            {
                toolbar->removeAllChildren();

                if (m_editMode)
                    m_editMode->configurePanelToolbar(this, panel, toolbar);
            }
        }
    }

    void ScenePreviewContainer::recreatePanelBottomToolbars()
    {
        for (auto& panel : m_panels)
        {
            if (const auto& toolbar = panel->bottomToolbar())
            {
                toolbar->removeAllChildren();

                if (m_editMode)
                    m_editMode->configurePanelToolbar(this, panel, toolbar);
            }
        }
    }

    void ScenePreviewContainer::requestRecreatePanelGizmos()
    {
        recreatePanelGizmos();
    }

    void ScenePreviewContainer::recreatePanelGizmos()
    {
        for (auto& panel : m_panels)
            panel->requestRecreateGizmo();
    }

    void ScenePreviewContainer::fillViewConfigMenu(ui::MenuButtonContainer* menu)
    {}

    void ScenePreviewContainer::focusNodes(const Array<SceneContentNodePtr>& nodes)
    {
        Box bounds;

        for (const auto& node : nodes)
        {
            Box nodeBounds;
            if (m_visualization->retrieveBoundsForProxy(node, nodeBounds))
            {
                bounds.merge(nodeBounds);
            }
        }

        focusBounds(bounds);
    }

    void ScenePreviewContainer::focusBounds(const Box& box)
    {
        if (!box.empty())
        {
            for (auto& panel : m_panels)
                panel->setupCameraAroundBounds(box);
        }
    }

    //--

} // ed
