/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "engine/world/include/world.h"
#include "engine/world/include/worldEntity.h"
#include "engine/rendering/include/stats.h"

BEGIN_BOOMER_NAMESPACE()

//--

/// a base "screen" (menu, gameplay, etc) that is being displayed to player in the game
class GAME_COMMON_API IGameScreen : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGameScreen, IObject);

public:
    IGameScreen();
    virtual ~IGameScreen();

    //--

    INLINE GameScreenStack* host() const { return m_host; }

    //--

    /// determine best fade-in time for this screen
    virtual float queryFadeInTime() const;

    /// determine best fade-out time for this screen
    virtual float queryFadeOutTime() const;

    /// determine if the game controller is opaque (hides all controlled lower on the stack)
    virtual bool queryOpaqueState() const;

    /// determine if this game controller should filter all input regardless if it was serviced or not
    virtual bool queryFilterAllInput() const;

    /// determine if user input should be captured (ie, hidden mouse, etc) - mostly the case when not paused
    virtual bool queryInputCaptureState() const;

    //--

    // called when we begin transition to this screen, good place to start loading, etc
    virtual void handleAttached();

    // called when screen got faded out and is being removed, good place to free memory
    virtual void handleDetached();

    // update game state, called before world update, can return false to indicate end of the game
    virtual void handleUpdate(double dt);

    // process high level input
    virtual bool handleInput(const InputEvent& evt);

    // render internal 
    virtual void handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, float visibility);

    // render ImGui debug overlay
    virtual void handleRenderImGuiDebugOverlay();

    //--

private:
    GameScreenStack* m_host = nullptr;

    friend class GameScreenStack;
};

//--

/// canvas based game screen
class IGameScreenCanvas : public IGameScreen
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGameScreenCanvas, IGameScreen);

public:
    IGameScreenCanvas();

    virtual void handleRender(Canvas& c, float visibility) = 0;

protected:
    virtual void handleRender(gpu::CommandWriter& cmd, const gpu::AcquiredOutput& output, float visibility) override final;
};

//--

END_BOOMER_NAMESPACE()
