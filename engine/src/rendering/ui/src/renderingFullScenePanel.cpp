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

        rendering::scene::SceneSetupInfo setup;
        setup.name = "RenderingPanelScene";
        setup.type = rendering::scene::SceneType::EditorPreview;

        m_scene = MemNew(rendering::scene::Scene, setup);
    }

    RenderingFullScenePanel::~RenderingFullScenePanel()
    {
        m_scene->releaseRef();
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

        if (m_scene)
        {
            auto& scene = frame.scenes.scenesToDraw.emplaceBack();
            scene.scenePtr = m_scene;
        }
    }

    //--

    void RenderingFullScenePanel::buildDefaultToolbar(IElement* owner, ToolBar* toolbar)
    {
        owner->actions().bindCommand("PreviewPanel.ChangeRenderMode"_id) = [owner, this]()
        {
            auto menu = base::CreateSharedPtr<MenuButtonContainer>();
            buildRenderModePopup(menu);
            menu->show(owner);
        };

        owner->actions().bindCommand("PreviewPanel.ChangeFilters"_id) = [owner, this]()
        {
            auto menu = base::CreateSharedPtr<MenuButtonContainer>();
            buildFilterPopup(menu);
            menu->show(owner);
        };

        toolbar->createButton("PreviewPanel.ChangeFilters"_id, ui::ToolbarButtonSetup().icon("eye").caption("Filters"));
        toolbar->createButton("PreviewPanel.ChangeRenderMode"_id, ui::ToolbarButtonSetup().icon("shader").caption("Render mode"));
        toolbar->createSeparator();
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

    void RenderingFullScenePanel::createFilterItem(base::StringView<char> prefix, const rendering::scene::FilterBitInfo* bitInfo, MenuButtonContainer* menu)
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

} // ui