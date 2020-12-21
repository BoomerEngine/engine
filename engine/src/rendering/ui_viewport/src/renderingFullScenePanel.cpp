/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: widgets #]
***/

#include "build.h"
#include "renderingFullScenePanel.h"
#include "rendering/scene/include/renderingScene.h"
#include "rendering/scene/include/renderingFrameParams.h"
#include "base/ui/include/uiMenuBar.h"
#include "base/ui/include/uiToolBar.h"

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_CLASS(RenderingFullScenePanel);
    RTTI_END_TYPE();

    //----

    RenderingFullScenePanel::RenderingFullScenePanel()
    {
        m_renderMode = rendering::scene::FrameRenderMode::Default;
        createToolbarItems();
    }

    RenderingFullScenePanel::~RenderingFullScenePanel()
    {
    }

    void RenderingFullScenePanel::handleUpdate(float dt)
    {
        TBaseClass::handleUpdate(dt);
    }

    void RenderingFullScenePanel::handleRender(rendering::scene::FrameParams& frame)
    {
        frame.mode = m_renderMode;
        frame.debug.materialDebugMode = m_renderMaterialDebugChannelName;

        TBaseClass::handleRender(frame);
    }

    //--

    void RenderingFullScenePanel::createToolbarItems()
    {
        actions().bindCommand("PreviewPanel.ChangeRenderMode"_id) = [this](ui::Button* button)
        {
            if (button)
            {
                auto menu = base::RefNew<MenuButtonContainer>();
                buildRenderModePopup(menu);
                menu->showAsDropdown(button);
            }
        };

        actions().bindCommand("PreviewPanel.ChangeFilters"_id) = [this](ui::Button* button)
        {
            if (button)
            {
                auto menu = base::RefNew<MenuButtonContainer>();
                buildFilterPopup(menu);
                menu->showAsDropdown(button);
            }
        };

        toolbar()->createButton("PreviewPanel.ChangeFilters"_id, ui::ToolbarButtonSetup().caption("[img:eye] Filters"));
        toolbar()->createButton("PreviewPanel.ChangeRenderMode"_id, ui::ToolbarButtonSetup().caption("[img:shader] Render mode"));
        toolbar()->createSeparator();
    }

    void RenderingFullScenePanel::configure(rendering::scene::FrameRenderMode mode, base::StringID materialChannelName /*= base::StringID::EMPTY()*/)
    {
        m_renderMode = mode;
        m_renderMaterialDebugChannelName = materialChannelName;
    }

    void RenderingFullScenePanel::buildRenderModePopup(ui::MenuButtonContainer* menu)
    {
        // default modes
        menu->createCallback("Default", "[img:viewmode]") = [this]() { configure(rendering::scene::FrameRenderMode::Default); };
        menu->createCallback("Solid Wireframe", "[img:cube]") = [this]() { configure(rendering::scene::FrameRenderMode::WireframeSolid); };
        menu->createCallback("Wire Wireframe", "[img:wireframe]") = [this]() { configure(rendering::scene::FrameRenderMode::WireframePassThrough); };
        menu->createSeparator();
        menu->createCallback("Debug depth", "[img:order_front]") = [this]() { configure(rendering::scene::FrameRenderMode::DebugDepth); };
        menu->createCallback("Debug luminance", "[img:lightbulb]") = [this]() { configure(rendering::scene::FrameRenderMode::DebugLuminance); };
        menu->createSeparator();

        // TODO: material debug
    }

    void RenderingFullScenePanel::createFilterItem(base::StringView prefix, const rendering::scene::FilterBitInfo* bitInfo, MenuButtonContainer* menu)
    {
        base::StringBuf name = prefix ? base::StringBuf(base::TempString("{}.{}", prefix, bitInfo->name)) : base::StringBuf(bitInfo->name.view());

        if (bitInfo->bit != rendering::scene::FilterBit::MAX)
        {
            const auto toggled = this->filterFlags().test(bitInfo->bit);

            menu->createCallback(name, toggled ? "[img:tick]" : "") = [this, bitInfo]()
            {
                filterFlags() ^= bitInfo->bit;
            };
        }

        for (const auto* child : bitInfo->children)
            createFilterItem(name, child, menu);
    }

    void RenderingFullScenePanel::buildFilterPopup(MenuButtonContainer* menu)
    {
        if (const auto* group = rendering::scene::GetFilterTree())
        {
            for (const auto* child : group->children)
            {
                menu->createSeparator();
                createFilterItem("", child, menu);
            }
        }
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(RenderingFullScenePanelWithScene);
    RTTI_END_TYPE();

    RenderingFullScenePanelWithScene::RenderingFullScenePanelWithScene()
    {
        const auto sceneType = rendering::scene::SceneType::EditorPreview;
        m_scene = base::RefNew<rendering::scene::Scene>(sceneType);
    }

    RenderingFullScenePanelWithScene::~RenderingFullScenePanelWithScene()
    {
        m_scene.reset();
    }

    void RenderingFullScenePanelWithScene::handleRender(rendering::scene::FrameParams& frame)
    {
        TBaseClass::handleRender(frame);
		frame.scenes.mainScenePtr = m_scene;
    }

    //--

} // ui