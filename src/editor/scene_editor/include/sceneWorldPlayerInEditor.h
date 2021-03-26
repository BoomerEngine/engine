/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: editor #]
***/

#pragma once

#include "engine/ui/include/uiRenderingPanel.h"
#include "engine/ui/include/uiDockPanel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//--

DECLARE_UI_EVENT(EVENT_PIE_GAME_FINISHED);

/// PIE viewport
class EDITOR_SCENE_EDITOR_API ScenePlayInEditorViewport : public ui::RenderingPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(ScenePlayInEditorViewport, ui::RenderingPanel);

public:
    ScenePlayInEditorViewport();
    virtual ~ScenePlayInEditorViewport();

    INLINE bool hasGame() const { return !m_screenStack.empty(); }

    bool pushScreen(IGameScreen* screen);
    bool pushGame(CompiledWorldDataPtr data, const CameraSetup& initCamera);
    void stop();

protected:
    ui::Timer m_updateTimer;

    GamePlatformPtr m_platform;
    GameScreenStackPtr m_screenStack;
    NativeTimePoint m_lastUpdateTime;

    void update(float dt);

    virtual void handleCamera(CameraSetup& outCamera) const override;
    virtual void handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, const CameraSetup& camera, const FrameParams_Capture* capture) override;
    virtual bool handleKeyEvent(const InputKeyEvent& evt) override;
    virtual ui::InputActionPtr handleMouseClick(const ui::ElementArea& area, const InputMouseClickEvent& evt) override;
    //virtual bool handleAxisEvent(const InputAxisEvent& evt) override;
    //virtual bool handleMouseMovement(const InputMouseMovementEvent& evt) override;
};

//--

/// PIE panel
class EDITOR_SCENE_EDITOR_API ScenePlayInEditorPanel : public ui::DockPanel
{
    RTTI_DECLARE_VIRTUAL_CLASS(ScenePlayInEditorPanel, ui::DockPanel);

public:
    ScenePlayInEditorPanel(StringView worldPath, SceneContentStructure* content, ScenePreviewContainer* preview);

    void stopGame();
    void startGame(CompiledWorldDataPtr data, const CameraSetup& initCamera);

    virtual void configLoad(const ui::ConfigBlock& block);
    virtual void configSave(const ui::ConfigBlock& block) const;

private:
    ui::ToolBarPtr m_toolbar;
    RefPtr<ScenePlayInEditorViewport> m_viewport;

    StringBuf m_worldPath;

    SceneContentStructure* m_content = nullptr;
    ScenePreviewContainer* m_preview = nullptr;

    bool m_playVisibleOnly = false;
    bool m_playFromCurrentPosition = true;
    bool m_startInSeparateWindow = false;

    void cmdPlay();
    void cmdStop();

    void updateToolbar();
};

//--

END_BOOMER_NAMESPACE_EX(ed)

