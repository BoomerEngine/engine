/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor\ui #]
***/

#include "build.h"
#include "sceneWorldPlayerInEditor.h"

#include "game/common/include/screenStack.h"
#include "game/common/include/screenSinglePlayerGame.h"
#include "game/common/include/settings.h"
#include "game/common/include/platformService.h"
#include "game/common/include/platformLocal.h"
#include "game/common/include/platform.h"

#include "engine/ui/include/uiElementConfig.h"
#include "engine/ui/include/uiToolBar.h"
#include "engine/ui/include/uiCheckBox.h"
#include "engine/ui/include/uiInputAction.h"
#include "engine/world_compiler/include/worldCompiler.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/framebuffer.h"

#include "core/app/include/projectSettingsService.h"

#include "editor/common/include/utils.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

ConfigProperty<float> cvPIEGameUpdateInterval("Editor", "PIEUpdateInterval", 1000.0f);

RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePlayInEditorViewport);
RTTI_END_TYPE();

ScenePlayInEditorViewport::ScenePlayInEditorViewport()
    : m_updateTimer(this, "Update"_id)
{
    m_platform = RefNew<GamePlatformLocal>();

    m_lastUpdateTime.resetToNow();
    m_updateTimer = [this]() {
        auto dt = m_lastUpdateTime.timeTillNow().toSeconds();
        m_lastUpdateTime.resetToNow();
        update(dt);
    };

    m_updateTimer.startRepeated(1.0f / cvPIEGameUpdateInterval.get());
}

ScenePlayInEditorViewport::~ScenePlayInEditorViewport()
{}

void ScenePlayInEditorViewport::update(float dt)
{
    if (m_screenStack)
    {
        if (!m_screenStack->update(dt))
        {
            m_screenStack.reset();
            call(EVENT_PIE_GAME_FINISHED);
        }
    }
}

void ScenePlayInEditorViewport::stop()
{
    if (m_screenStack)
    {
        m_screenStack->pushTransition(nullptr, GameTransitionMode::ReplaceAll, true);
        m_screenStack->update(0.1f);
        m_screenStack->update(0.1f);
        m_screenStack->update(0.1f);
        m_screenStack.reset();
    }
}

bool ScenePlayInEditorViewport::pushScreen(IGameScreen* screen)
{
    if (!m_screenStack)
        m_screenStack = RefNew<GameScreenStack>(m_platform);

    m_screenStack->pushTransition(screen, GameTransitionMode::ReplaceAll, true);
    return true;
}

bool ScenePlayInEditorViewport::pushGame(CompiledWorldDataPtr data, const CameraSetup& initCamera)
{
    const auto settings = GetSettings<GameProjectSettingScreens>();
    if (settings->m_singlePlayerGameScreen)
    {
        if (auto screen = settings->m_singlePlayerGameScreen->create<IGameScreenSinglePlayer>())
        {
            GameScreenSinglePlayerSetup setup;
            setup.localPlayerIdentity = GetService<GamePlatformService>()->platform()->queryLocalPlayer(0); // TODO: editor player identity
            setup.worldData = data;

            if (screen->initialize(setup))
            {
                return pushScreen(screen);
            }
        }
    }

    return false;
}

void ScenePlayInEditorViewport::handleCamera(CameraSetup& outCamera) const
{

}

class ScenePlayInEditorViewportInputAction : public ui::IInputAction
{
public:
    ScenePlayInEditorViewportInputAction(ScenePlayInEditorViewport* viewport, GameScreenStack* screenStack)
        : IInputAction(viewport)
        , m_stack(AddRef(screenStack))
    {}

    virtual ui::InputActionResult onKeyEvent(const input::KeyEvent& evt) override
    {
        if (evt.pressed() && evt.keyCode() == input::KeyCode::KEY_ESCAPE)
            return nullptr;

        m_stack->input(evt);
        return ui::InputActionResult();
    }

    virtual ui::InputActionResult onAxisEvent(const input::AxisEvent& evt)
    {
        m_stack->input(evt);
        return ui::InputActionResult();
    }

    virtual ui::InputActionResult onMouseEvent(const input::MouseClickEvent& evt, const ui::ElementWeakPtr& hoveredElement) override
    {
        m_stack->input(evt);
        return ui::InputActionResult();
    }

    virtual ui::InputActionResult onMouseMovement(const input::MouseMovementEvent& evt, const ui::ElementWeakPtr& hoveredElement) override
    {
        m_stack->input(evt);
        return ui::InputActionResult();
    }

    virtual void onCanceled() override
    {

    }

    virtual bool allowsHoverTracking() const override
    {
        return false;
    }

    virtual bool fullMouseCapture() const override
    {
        return true;
    }

private:
    GameScreenStackPtr m_stack;
};

ui::InputActionPtr ScenePlayInEditorViewport::handleMouseClick(const ui::ElementArea& area, const input::MouseClickEvent& evt)
{
    return RefNew<ScenePlayInEditorViewportInputAction>(this, m_screenStack);
}

bool ScenePlayInEditorViewport::handleKeyEvent(const input::KeyEvent& evt)
{
    if (m_screenStack)
        if (m_screenStack->input(evt))
            return true;

    return TBaseClass::handleKeyEvent(evt);
}

/*bool ScenePlayInEditorViewport::handleAxisEvent(const input::AxisEvent& evt)
{
    if (m_screenStack)
        if (m_screenStack->input(evt))
            return true;

    return TBaseClass::handleAxisEvent(evt);
}

bool ScenePlayInEditorViewport::handleMouseMovement(const input::MouseMovementEvent& evt)
{
    if (m_screenStack)
        if (m_screenStack->input(evt))
            return true;

    return TBaseClass::handleMouseMovement(evt);
}*/

void ScenePlayInEditorViewport::handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const CameraSetup& camera, const rendering::FrameParams_Capture* capture)
{
    if (m_screenStack)
    {
        m_screenStack->render(cmd, output);
    }
    else
    {
        gpu::FrameBuffer fb;
        fb.color[0].view(output.color).clear(Color(20, 20, 20, 255));
        cmd.opBeingPass(fb);
        cmd.opEndPass();
    }
}

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(ScenePlayInEditorPanel);
RTTI_END_TYPE();

ScenePlayInEditorPanel::ScenePlayInEditorPanel(StringView worldPath, SceneContentStructure* content, ScenePreviewContainer* preview)
    : ui::DockPanel("[img:controller] Play", "PIE")
    , m_content(content)
    , m_preview(preview)
    , m_worldPath(worldPath)
{
    layoutVertical();

    m_toolbar = createChild<ui::ToolBar>();

    m_toolbar->createButton(ui::ToolBarButtonInfo("Play"_id).caption("Play", "controller")) = [this]() { cmdPlay(); };
    m_toolbar->createButton(ui::ToolBarButtonInfo("Stop"_id).caption("Stop", "cancel")) = [this]() { cmdStop(); };

    m_viewport = createChild<ScenePlayInEditorViewport>();
    m_viewport->expand();
    m_viewport->bind(EVENT_PIE_GAME_FINISHED) = [this]()
    {
        updateToolbar();
    };
}

void ScenePlayInEditorPanel::configLoad(const ui::ConfigBlock& block)
{
    block.read("PlayVisibleOnly", m_playVisibleOnly);
    block.read("PlayFromCurrentCameraVisibleOnly", m_playFromCurrentPosition);
    block.read("StartSeparteWindow", m_startInSeparateWindow);
}

void ScenePlayInEditorPanel::configSave(const ui::ConfigBlock& block) const
{
    block.write("PlayVisibleOnly", m_playVisibleOnly);
    block.write("PlayFromCurrentCameraVisibleOnly", m_playFromCurrentPosition);
    block.write("StartSeparteWindow", m_startInSeparateWindow);
}

void ScenePlayInEditorPanel::updateToolbar()
{
    m_toolbar->enableButton("Play"_id, !m_viewport->hasGame());
    m_toolbar->enableButton("Stop"_id, m_viewport->hasGame());
}

void ScenePlayInEditorPanel::cmdPlay()
{
    DEBUG_CHECK_RETURN_EX(m_viewport, "Already playing");

    CompiledWorldDataPtr world;
    RunLongAction(this, nullptr, [this, &world](IProgressTracker& progress)
        {
            world = CompileRawWorld(m_worldPath, progress);
        });

    if (world)
        startGame(world, CameraSetup());
    else
        ui::PostWindowMessage(this, ui::MessageType::Error, "PIE"_id, "Unable to compile world");
}

void ScenePlayInEditorPanel::cmdStop()
{
    stopGame();
}

void ScenePlayInEditorPanel::stopGame()
{
    m_viewport->stop();
    updateToolbar();
}

void ScenePlayInEditorPanel::startGame(CompiledWorldDataPtr data, const CameraSetup& initCamera)
{
    m_viewport->stop();
    m_viewport->pushGame(data, initCamera);
    updateToolbar();
}

//--

END_BOOMER_NAMESPACE_EX(ed)
