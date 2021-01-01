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

    static base::StringView RenderModeString(rendering::scene::FrameRenderMode mode)
    {
        switch (mode)
        {
            case rendering::scene::FrameRenderMode::Default: return "[img:viewmode] Default";
            case rendering::scene::FrameRenderMode::WireframeSolid: return "[img:cube] Solid Wireframe";
            case rendering::scene::FrameRenderMode::WireframePassThrough: return "[img:wireframe] Wire Wireframe";
            case rendering::scene::FrameRenderMode::DebugDepth: return "[img:order_front] Debug Depth";
            case rendering::scene::FrameRenderMode::DebugLuminance: return "[img:lightbulb] Debug Luminance";
            case rendering::scene::FrameRenderMode::DebugShadowMask: return "[img:shadow_on] Debug Shadowmask";
            case rendering::scene::FrameRenderMode::DebugAmbientOcclusion: return "[img:shader] Debug Ambient Occlusion";
            case rendering::scene::FrameRenderMode::DebugReconstructedViewNormals: return "[img:axis] Debug Normals";
        }

        return "";
    }

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
        toolbar()->createButton("PreviewPanel.ChangeRenderMode"_id, ui::ToolbarButtonSetup().caption(RenderModeString(m_renderMode)));
        toolbar()->createSeparator();
    }

    void RenderingFullScenePanel::configure(rendering::scene::FrameRenderMode mode, base::StringID materialChannelName /*= base::StringID::EMPTY()*/)
    {
        m_renderMode = mode;
        m_renderMaterialDebugChannelName = materialChannelName;

        toolbar()->updateButtonCaption("PreviewPanel.ChangeRenderMode"_id, ui::ToolbarButtonSetup().caption(RenderModeString(m_renderMode)));
    }

    static const rendering::scene::FrameRenderMode USER_SELECTABLE_RENDER_MODES[] = {
        rendering::scene::FrameRenderMode::Default,
        rendering::scene::FrameRenderMode::WireframeSolid,
        rendering::scene::FrameRenderMode::WireframePassThrough,
        rendering::scene::FrameRenderMode::DebugDepth,
        rendering::scene::FrameRenderMode::DebugLuminance,
        rendering::scene::FrameRenderMode::DebugShadowMask,
        rendering::scene::FrameRenderMode::DebugReconstructedViewNormals,
        rendering::scene::FrameRenderMode::DebugAmbientOcclusion,
    };

    void RenderingFullScenePanel::buildRenderModePopup(ui::MenuButtonContainer* menu)
    {
        for (auto mode : USER_SELECTABLE_RENDER_MODES)
        {
            if (mode == rendering::scene::FrameRenderMode::DebugDepth)
                menu->createSeparator();

            if (auto title = RenderModeString(mode))
                menu->createCallback(RenderModeString(mode)) = [this, mode]() { configure(mode); };
        }

        menu->createSeparator();
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