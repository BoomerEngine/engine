/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "screen.h"
#include "screenLoadingScreen.h"

#include "engine/canvas/include/geometryBuilder.h"
#include "engine/canvas/include/canvas.h"

BEGIN_BOOMER_NAMESPACE()

ConfigProperty<float> cvLoadingScreenFadeInTime("Game", "LoadingScreenFadeInTime", 0.2f);
ConfigProperty<float> cvLoadingScreenFadeOutTime("Game", "LoadingScreenFadeOutTime", 0.2f);

//--

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGameLoadingScreen);
RTTI_END_TYPE();

IGameLoadingScreen::IGameLoadingScreen()
{
}

IGameLoadingScreen::~IGameLoadingScreen()
{
}

void IGameLoadingScreen::makeTransparent()
{
    m_opaque = false;
}

float IGameLoadingScreen::queryFadeInTime() const
{
    return cvLoadingScreenFadeInTime.get();
}
    
float IGameLoadingScreen::queryFadeOutTime() const
{
    return cvLoadingScreenFadeOutTime.get();
}

bool IGameLoadingScreen::queryOpaqueState() const
{
    return m_opaque;
}

bool IGameLoadingScreen::queryFilterAllInput() const
{
    return m_opaque;
}

bool IGameLoadingScreen::queryInputCaptureState() const
{
    return true;
}

//--

RTTI_BEGIN_TYPE_CLASS(GameLoadingScreenSimple);
RTTI_END_TYPE();

GameLoadingScreenSimple::GameLoadingScreenSimple()
{}

GameLoadingScreenSimple::~GameLoadingScreenSimple()
{}

void GameLoadingScreenSimple::handleUpdate(double dt)
{
    m_totalTime += dt;
}

void GameLoadingScreenSimple::handleRender(Canvas& c, float visibility)
{
    InplaceGeometryBuilder g(c);
    g.solidRect(Color::BLACK, 0, 0, c.width(), c.height());
    g.render();

    c.debugPrint(c.width() - 50.0f, c.height() - 50.0f, "Loading...", 20, Color::WHITE, FontAlignmentHorizontal::Right);
}

//--


END_BOOMER_NAMESPACE()
