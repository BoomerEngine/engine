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

/// loading screen helper
class GAME_COMMON_API IGameLoadingScreen : public IGameScreenCanvas
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGameLoadingScreen, IGameScreenCanvas);

public:
    IGameLoadingScreen();
    virtual ~IGameLoadingScreen();

    void makeTransparent();

protected:
    virtual float queryFadeInTime() const override;
    virtual float queryFadeOutTime() const override;
    virtual bool queryOpaqueState() const override;
    virtual bool queryFilterAllInput() const override;
    virtual bool queryInputCaptureState() const override;

    bool m_opaque = true;
};

//--

/// simples loading screen
class GAME_COMMON_API GameLoadingScreenSimple : public IGameLoadingScreen
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameLoadingScreenSimple, IGameLoadingScreen);

public:
    GameLoadingScreenSimple();
    virtual ~GameLoadingScreenSimple();

private:
    virtual void handleUpdate(double dt) override final;
    virtual void handleRender(Canvas& c, float visibility) override final;

    float m_totalTime = 0.0f;
};

//--


END_BOOMER_NAMESPACE()
