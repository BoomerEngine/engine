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
#include "sceneContentNodes.h"
#include "sceneContentNodesEntity.h"

#include "engine/rendering/include/scene.h"
#include "engine/rendering/include/debug.h"
#include "engine/rendering/include/params.h"

#include "engine/ui/include/uiToolBar.h"
#include "engine/world/include/world.h"
#include "engine/ui/include/uiElementConfig.h"
#include "engine/ui/include/uiSplitter.h"
#include "engine/ui/include/uiFourWaySplitter.h"
#include "engine/ui/include/uiMenuBar.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

RTTI_BEGIN_TYPE_CLASS(SceneGridSettings);
    RTTI_PROPERTY(positionGridEnabled);
    RTTI_PROPERTY(rotationGridEnabled);
    RTTI_PROPERTY(positionGridSize);
    RTTI_PROPERTY(rotationGridSize);
    RTTI_PROPERTY(featureSnappingEnabled);
    RTTI_PROPERTY(rotationSnappingEnabled);
    RTTI_PROPERTY(snappingDistance);
RTTI_END_TYPE();

SceneGridSettings::SceneGridSettings()
{}

//--

RTTI_BEGIN_TYPE_CLASS(SceneGizmoSettings);
    RTTI_PROPERTY(mode);
    RTTI_PROPERTY(space);
    RTTI_PROPERTY(target);
    RTTI_PROPERTY(enableX);
    RTTI_PROPERTY(enableY);
    RTTI_PROPERTY(enableZ);
RTTI_END_TYPE();

SceneGizmoSettings::SceneGizmoSettings()
{}

//--

RTTI_BEGIN_TYPE_CLASS(SceneSelectionSettings);
RTTI_PROPERTY(explorePrefabs);
RTTI_PROPERTY(allowTransparent);
RTTI_END_TYPE();

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

RTTI_BEGIN_TYPE_CLASS(SceneLayoutViewportSettings);
    RTTI_PROPERTY(camera);
    RTTI_PROPERTY(panel);
RTTI_END_TYPE();

SceneLayoutViewportSettings::SceneLayoutViewportSettings()
{}

//--

RTTI_BEGIN_TYPE_ENUM(SceneViewportLayout);
RTTI_ENUM_OPTION(Single)
    RTTI_ENUM_OPTION(ClassVertical)
    RTTI_ENUM_OPTION(ClassHorizontal)
    RTTI_ENUM_OPTION(ClassFourWay)
    RTTI_ENUM_OPTION(TwoSmallOneBigLeft);
    RTTI_ENUM_OPTION(TwoSmallOneBigTop);
    RTTI_ENUM_OPTION(TwoSmallOneBigRight);
    RTTI_ENUM_OPTION(TwoSmallOneBigBottom);
    RTTI_ENUM_OPTION(ThreeSmallOneBigLeft);
    RTTI_ENUM_OPTION(ThreeSmallOneBigTop);
    RTTI_ENUM_OPTION(ThreeSmallOneBigRight);
    RTTI_ENUM_OPTION(ThreeSmallOneBigBottom);
RTTI_END_TYPE();

RTTI_BEGIN_TYPE_CLASS(SceneLayoutSettings);
    RTTI_PROPERTY(layout);
    RTTI_PROPERTY(splitterState);
    RTTI_PROPERTY(viewportState);
RTTI_END_TYPE();
    
SceneLayoutSettings::SceneLayoutSettings()
{
    splitterState[Splitter_MainVertical] = 0.5f;
    splitterState[Splitter_MainHorizontal] = 0.5f;
    splitterState[Splitter_AdditionalPrimaryLeft] = 0.33f;
    splitterState[Splitter_AdditionalPrimaryTop] = 0.33f;
    splitterState[Splitter_AdditionalPrimaryRight] = 0.66f;
    splitterState[Splitter_AdditionalPrimaryBottom] = 0.66f;
    splitterState[Splitter_AdditionalSecondaryVertical1] = 0.33f;
    splitterState[Splitter_AdditionalSecondaryVertical2] = 0.66f;
    splitterState[Splitter_AdditionalSecondaryHorizontal1] = 0.33f;
    splitterState[Splitter_AdditionalSecondaryHorizontal2] = 0.66f;

    viewportState[Viewport_Main].camera.mode = ui::CameraMode::FreePerspective;
    viewportState[Viewport_Extra1].camera.mode = ui::CameraMode::Top;
    viewportState[Viewport_Extra2].camera.mode = ui::CameraMode::Front;
    viewportState[Viewport_Extra3].camera.mode = ui::CameraMode::Right;

    viewportState[Viewport_Main].camera.rotation = Angles(20.0f, 40.0f, 0.0f);
    viewportState[Viewport_Main].camera.position = -viewportState[Viewport_Main].camera.rotation.forward() * 3.0f;

    viewportState[Viewport_Main].panel.renderMode = rendering::FrameRenderMode::Default;
    viewportState[Viewport_Extra1].panel.renderMode = rendering::FrameRenderMode::WireframePassThrough;
    viewportState[Viewport_Extra2].panel.renderMode = rendering::FrameRenderMode::WireframePassThrough;
    viewportState[Viewport_Extra3].panel.renderMode = rendering::FrameRenderMode::WireframePassThrough;
}

//--
     
RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePreviewContainer);
RTTI_END_TYPE();

ScenePreviewContainer::ScenePreviewContainer(SceneContentStructure* content, ISceneEditMode* initialEditMode, RawWorldData* worldData)
    : m_content(content)
{
    createWorld(worldData);

    m_bottomBar = RefNew<ui::ToolBar>();
    m_bottomBar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);

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
    if (!node)
        return false;

    if (node && node->type() == SceneContentNodeType::Entity)
    {
        const auto* entityNode = static_cast<const SceneContentEntityNode*>(node);
        if (!entityNode->baseData().empty())
            return true;
    }

    return false;
}

SceneContentNodePtr ScenePreviewContainer::resolveSelectable(const Selectable& selectable, bool raw /*= false*/) const
{
    auto node = m_visualization->resolveSelectable(selectable);

    if (node && !raw)
    {
        if (!m_selectionSettings.explorePrefabs)
        {
            while (IsDevivedNode(node->parent()))
                node = AddRef(node->parent());                    
        }
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

        for (const auto& panel : m_viewports)
        {
            if (panel)
            {
                panel->bindEditMode(nullptr);

                if (const auto& toolbar = panel->bottomToolbar())
                    toolbar->removeAllChildren();
            }
        }
    }
}

void ScenePreviewContainer::activateEditMode()
{
    if (m_editMode)
    {
        m_editMode->acivate(this);

        for (const auto& panel : m_viewports)
            if (panel)
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

void ScenePreviewContainer::changeViewportLayout(SceneViewportLayout layout)
{
    if (m_layoutSettings.layout != layout)
    {
        destroyPanels();
        m_layoutSettings.layout = layout;
        createPanels();
    }
}

void ScenePreviewContainer::configSave(const ui::ConfigBlock& block) const
{
    TBaseClass::configSave(block);

    auto layout = m_layoutSettings;
    captureLayoutSettings(layout);

    block.write("gridSettings", m_gridSettings);
    block.write("gizmoSettings", m_gizmoSettings);
    block.write("selectionSettings", m_selectionSettings);
    block.write("layoutSettings", layout);
}

void ScenePreviewContainer::configLoad(const ui::ConfigBlock& block)
{
    block.read("gridSettings", m_gridSettings);
    block.read("gizmoSettings", m_gizmoSettings);
    block.read("selectionSettings", m_selectionSettings);
    block.read("layoutSettings", m_layoutSettings);

    createPanels();
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
        PC_SCOPE_LVL0(SyncProxies);

        Array<SceneContentSyncOp> syncNodes;
        m_content->syncNodeChanges(syncNodes);

        for (const auto& sync : syncNodes)
        {
            switch (sync.type)
            {
                case SceneContentSyncOpType::Added:
                    m_visualization->createProxy(sync.node);
                    break;

                case SceneContentSyncOpType::Removed:
                    m_visualization->removeProxy(sync.node);
                    break;
            }
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

    if (m_editMode)
        m_editMode->handleUpdate(dt);

    m_world->update(dt);
}

void ScenePreviewContainer::createWorld(RawWorldData* data)
{
    if (data)
        m_world = World::CreateEditorWorld(data);
    else
        m_world = World::CreatePreviewWorld();
    m_lastWorldTick.resetToNow();

    m_visualization = RefNew<SceneNodeVisualizationHandler>(m_world);
}

//--

void ScenePreviewContainer::destroyPanels()
{
    captureLayoutSettings(m_layoutSettings);

    for (auto& panel : m_viewports)
    {
        if (panel)
            panel->bindEditMode(nullptr);
        panel.reset();
    }

    for (auto& splitter : m_splitters)
    {
        if (splitter)
            splitter->removeAllChildren();
        splitter.reset();
    }

    removeAllChildren();
}

void ScenePreviewContainer::captureLayoutSettings(SceneLayoutSettings& outSettings) const
{
    for (uint32_t i = 0; i < ARRAY_COUNT(m_splitters); ++i)
    {
        if (auto standardSplitter = rtti_cast<ui::Splitter>(m_splitters[i]))
        {
            outSettings.splitterState[i] = standardSplitter->splitFraction();
        }
        else if (auto fourWaySplitter = rtti_cast<ui::FourWaySplitter>(m_splitters[i]))
        {
            outSettings.splitterState[i] = fourWaySplitter->splitFraction().x;
            outSettings.splitterState[i+1] = fourWaySplitter->splitFraction().y;
        }
    }

    for (uint32_t i = 0; i < ARRAY_COUNT(m_viewports); ++i)
    {
        if (auto viewport = m_viewports[i])
        {
            outSettings.viewportState[i].camera = viewport->cameraSettings();
            outSettings.viewportState[i].panel = viewport->panelSettings();
        }
    }
}

void ScenePreviewContainer::createPanels()
{
    destroyPanels();

    //--
        
    switch (m_layoutSettings.layout)
    {
        case SceneViewportLayout::Single:
        {
            m_viewports[SceneLayoutSettings::Viewport_Main] = createChild<ScenePreviewPanel>(this);
            break;
        }

        case SceneViewportLayout::ClassVertical:
        {
            m_splitters[SceneLayoutSettings::Splitter_MainVertical] = createChild<ui::Splitter>(ui::Direction::Vertical, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainVertical]);
            m_viewports[SceneLayoutSettings::Viewport_Main] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            m_viewports[SceneLayoutSettings::Viewport_Extra1] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            break;
        }

        case SceneViewportLayout::ClassHorizontal:
        {
            m_splitters[SceneLayoutSettings::Splitter_MainVertical] = createChild<ui::Splitter>(ui::Direction::Horizontal, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainVertical]);
            m_viewports[SceneLayoutSettings::Viewport_Main] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            m_viewports[SceneLayoutSettings::Viewport_Extra1] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            break;
        }

        case SceneViewportLayout::ClassFourWay:
        {
            m_splitters[SceneLayoutSettings::Splitter_MainVertical] = createChild<ui::FourWaySplitter>(m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainVertical], m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainHorizontal]);
            m_viewports[SceneLayoutSettings::Viewport_Main] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            m_viewports[SceneLayoutSettings::Viewport_Extra1] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            m_viewports[SceneLayoutSettings::Viewport_Extra2] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            m_viewports[SceneLayoutSettings::Viewport_Extra3] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            break;
        }

        case SceneViewportLayout::ThreeSmallOneBigLeft:
        case SceneViewportLayout::TwoSmallOneBigLeft:
        {
            m_splitters[SceneLayoutSettings::Splitter_MainVertical] = createChild<ui::Splitter>(ui::Direction::Vertical, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainVertical]);
            m_viewports[SceneLayoutSettings::Viewport_Main] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);

            m_splitters[SceneLayoutSettings::Splitter_MainHorizontal] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ui::Splitter>(ui::Direction::Horizontal, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainHorizontal]);
            m_viewports[SceneLayoutSettings::Viewport_Extra1] = m_splitters[SceneLayoutSettings::Splitter_MainHorizontal]->createChild<ScenePreviewPanel>(this);
            m_viewports[SceneLayoutSettings::Viewport_Extra2] = m_splitters[SceneLayoutSettings::Splitter_MainHorizontal]->createChild<ScenePreviewPanel>(this);
            break;
        }

        case SceneViewportLayout::ThreeSmallOneBigRight:
        case SceneViewportLayout::TwoSmallOneBigRight:
        {
            m_splitters[SceneLayoutSettings::Splitter_MainVertical] = createChild<ui::Splitter>(ui::Direction::Vertical, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainVertical]);

            m_splitters[SceneLayoutSettings::Splitter_MainHorizontal] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ui::Splitter>(ui::Direction::Horizontal, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainHorizontal]);
            m_viewports[SceneLayoutSettings::Viewport_Extra1] = m_splitters[SceneLayoutSettings::Splitter_MainHorizontal]->createChild<ScenePreviewPanel>(this);
            m_viewports[SceneLayoutSettings::Viewport_Extra2] = m_splitters[SceneLayoutSettings::Splitter_MainHorizontal]->createChild<ScenePreviewPanel>(this);

            m_viewports[SceneLayoutSettings::Viewport_Main] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);

            break;
        }

        case SceneViewportLayout::ThreeSmallOneBigTop:
        case SceneViewportLayout::TwoSmallOneBigTop:
        {
            m_splitters[SceneLayoutSettings::Splitter_MainHorizontal] = createChild<ui::Splitter>(ui::Direction::Horizontal, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainHorizontal]);
            m_viewports[SceneLayoutSettings::Viewport_Main] = m_splitters[SceneLayoutSettings::Splitter_MainHorizontal]->createChild<ScenePreviewPanel>(this);

            m_splitters[SceneLayoutSettings::Splitter_MainVertical] = m_splitters[SceneLayoutSettings::Splitter_MainHorizontal]->createChild<ui::Splitter>(ui::Direction::Vertical, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainVertical]);
            m_viewports[SceneLayoutSettings::Viewport_Extra1] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            m_viewports[SceneLayoutSettings::Viewport_Extra2] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            break;
        }

        case SceneViewportLayout::ThreeSmallOneBigBottom:
        case SceneViewportLayout::TwoSmallOneBigBottom:
        {
            m_splitters[SceneLayoutSettings::Splitter_MainHorizontal] = createChild<ui::Splitter>(ui::Direction::Horizontal, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainHorizontal]);

            m_splitters[SceneLayoutSettings::Splitter_MainVertical] = m_splitters[SceneLayoutSettings::Splitter_MainHorizontal]->createChild<ui::Splitter>(ui::Direction::Vertical, m_layoutSettings.splitterState[SceneLayoutSettings::Splitter_MainVertical]);
            m_viewports[SceneLayoutSettings::Viewport_Extra1] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);
            m_viewports[SceneLayoutSettings::Viewport_Extra2] = m_splitters[SceneLayoutSettings::Splitter_MainVertical]->createChild<ScenePreviewPanel>(this);

            m_viewports[SceneLayoutSettings::Viewport_Main] = m_splitters[SceneLayoutSettings::Splitter_MainHorizontal]->createChild<ScenePreviewPanel>(this);
            break;
        }
    }

    //--

    for (uint32_t i=0; i<ARRAY_COUNT(m_viewports); ++i)
    {
        if (auto panel = m_viewports[i])
        {
            panel->expand();

            panel->cameraSettings(m_layoutSettings.viewportState[i].camera);
            panel->panelSettings(m_layoutSettings.viewportState[i].panel);

            if (m_editMode)
                panel->bindEditMode(m_editMode);

            /*if (const auto& toolbar = panel->bottomToolbar())
            {
                toolbar->removeAllChildren();

                if (m_editMode)
                    m_editMode->configurePanelToolbar(this, panel, toolbar);
            }*/
        }
    }

    //--

    attachChild(m_bottomBar);
}

void ScenePreviewContainer::recreatePanelBottomToolbars()
{
    /*for (auto& panel : m_viewports)
    {
        if (panel)
        {
            if (const auto& toolbar = panel->bottomToolbar())
            {
                toolbar->removeAllChildren();

                if (m_editMode)
                    m_editMode->configurePanelToolbar(this, panel, toolbar);
            }
        }
    }*/

    m_bottomBar->removeAllChildren();

    if (m_editMode)
        m_editMode->configurePanelToolbar(this, nullptr, m_bottomBar);
}

void ScenePreviewContainer::requestRecreatePanelGizmos()
{
    recreatePanelGizmos();
}

void ScenePreviewContainer::recreatePanelGizmos()
{
    for (auto& panel : m_viewports)
        if (panel)
            panel->requestRecreateGizmo();
}

static const SceneViewportLayout USER_SELECTABLE_VIEWPORT_LAYOUTS[] = {
    SceneViewportLayout::Single,
    SceneViewportLayout::ClassVertical,
    SceneViewportLayout::ClassHorizontal,
    SceneViewportLayout::ClassFourWay,
    SceneViewportLayout::TwoSmallOneBigLeft,
    SceneViewportLayout::TwoSmallOneBigTop,
    SceneViewportLayout::TwoSmallOneBigRight,
    SceneViewportLayout::TwoSmallOneBigBottom,
    SceneViewportLayout::ThreeSmallOneBigLeft,
    SceneViewportLayout::ThreeSmallOneBigTop,
    SceneViewportLayout::ThreeSmallOneBigRight,
    SceneViewportLayout::ThreeSmallOneBigBottom,
};

static StringView ViewportLayoutText(SceneViewportLayout layout)
{
    switch (layout)
    {
        case SceneViewportLayout::Single: return "Single";
        case SceneViewportLayout::ClassVertical: return "2 Vertical";
        case SceneViewportLayout::ClassHorizontal: return "1 Horizontal";
        case SceneViewportLayout::ClassFourWay: return "2x2 Four Way";
        case SceneViewportLayout::TwoSmallOneBigLeft: return "Left 1x2";
        case SceneViewportLayout::TwoSmallOneBigTop: return "Top 1x2";
        case SceneViewportLayout::TwoSmallOneBigRight: return "Right 1x2";
        case SceneViewportLayout::TwoSmallOneBigBottom: return "Bottom 1x2";
        case SceneViewportLayout::ThreeSmallOneBigLeft: return "Left 1x3";
        case SceneViewportLayout::ThreeSmallOneBigTop: return "Top 1x3";
        case SceneViewportLayout::ThreeSmallOneBigRight: return "Right 1x3";
        case SceneViewportLayout::ThreeSmallOneBigBottom: return "Bottom 1x3";
    }

    return "";
}

void ScenePreviewContainer::fillViewConfigMenu(ui::MenuButtonContainer* menu)
{
    menu->createSeparator();

    for (const auto mode : USER_SELECTABLE_VIEWPORT_LAYOUTS)
    {
        if (mode == SceneViewportLayout::ClassVertical || mode == SceneViewportLayout::TwoSmallOneBigLeft || mode == SceneViewportLayout::ThreeSmallOneBigLeft)
            menu->createSeparator();

        if (auto title = ViewportLayoutText(mode))
            menu->createCallback(title, "[img:table]") = [this, mode]() {
            changeViewportLayout(mode);
        };
    }

    menu->createSeparator();
}

bool ScenePreviewContainer::calcNodesVisualBounds(const SceneContentNode* node, Box& outBounds) const
{
    bool valid = false;

    Box nodeBounds;
    if (m_visualization->retrieveBoundsForProxy(node, nodeBounds))
    {
        outBounds.merge(nodeBounds);
        valid = true;
    }

    for (const auto& child : node->children())
        valid |= calcNodesVisualBounds(child, outBounds);

    return valid;
}

bool ScenePreviewContainer::calcNodesVisualBounds(const Array<SceneContentNodePtr>& nodes, Box& outBounds) const
{
    bool validBounds = false;

    for (const auto& node : nodes)
    {
        Box nodeBounds;
        if (calcNodesVisualBounds(node, nodeBounds))
        {
            outBounds.merge(nodeBounds);
            validBounds = true;
        }
    }

    return validBounds;
}

void ScenePreviewContainer::focusNodes(const Array<SceneContentNodePtr>& nodes)
{
    Box bounds;
    if (calcNodesVisualBounds(nodes, bounds))
        focusBounds(bounds);
}

void ScenePreviewContainer::focusBounds(const Box& box)
{
    if (!box.empty())
    {
        for (auto& panel : m_viewports)
            if (panel)
                panel->focusOnBounds(box, 1.0f, nullptr);
    }
}

//--

END_BOOMER_NAMESPACE_EX(ed)
