/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//--

class FreeCameraHelper;

//--

// setup for rendering of the game screen
struct HostViewport
{
    // NOTE: may change
    uint32_t width = 0;
    uint32_t height = 0;
    const gpu::RenderTargetView* backBufferColor = nullptr;
	const gpu::RenderTargetView* backBufferDepth = nullptr;
};

//--

// type of host
enum class HostType : uint8_t
{
    Standalone, // standalone, actual game
    InEditor, // in-editor game
    Server, // headless server
};

//--

/// Container class for master game project, worlds, scenes and everything else
/// Can be instanced as a standalone app/project or in editor
/// This class also provides the highest (global) level of scripting in the game - allows to write stuff like loading screens, settings panel, world selection etc
/// This class is NOT intended to be derived from, internal flow is managed with "game screens"
class ENGINE_GAME_API Host : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(Host, IObject);

public:
    Host(HostType type, const GamePtr& game);
    virtual ~Host();

    //-- 

    // get type of the game host
    INLINE HostType type() const { return m_type; }

    // get the game that we are running
    INLINE const GamePtr& game() const { return m_game; }

    //--

    // update with given engine time delta, returns false if there are no screens to update (game finished)
    bool update(double dt);

    // render all required content, all command should be recorded via provided command buffer writer
    void render(gpu::CommandWriter& cmd, const HostViewport& viewport);

    // service input message
    bool input(const input::BaseEvent& evt);

    // should the host have exclusive access to user input?
    bool shouldCaptureInput() const;

    //--

private:
    HostType m_type;
    GamePtr m_game;

    ImGui::ImGUICanvasHelper* m_imgui = nullptr;
    bool m_paused = false;

    float m_loadingScreenVisibility = 0.0f;
    bool m_loadingScreenRequired = false;

    double m_gameAccumulatedTime = 0.0;
    NativeTimePoint m_startTime;

    gpu::ImageObjectPtr m_flippedColorTarget;
    gpu::RenderTargetViewPtr m_flippedColorTargetRTV;

    gpu::ImageObjectPtr m_flippedDepthTarget;
    gpu::RenderTargetViewPtr m_flippedDepthTargetRTV;

    rendering::CameraContextPtr m_cameraContext;

    uint32_t m_frameIndex = 0;

    bool processDebugInput(const input::BaseEvent& evt);

    void renderOverlay(gpu::CommandWriter& cmd, const HostViewport& viewport);
    void renderCanvas(canvas::Canvas& canvas);
};

//--

END_BOOMER_NAMESPACE()
