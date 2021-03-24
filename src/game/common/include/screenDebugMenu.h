/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

#include "screen.h"

BEGIN_BOOMER_NAMESPACE()

//--

class IGameScreenDebugMenu;

/// A simple debug option
class GAME_COMMON_API IDebugMenuElement : public IObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(IDebugMenuElement, IObject);

public:
    IDebugMenuElement(StringID action);
    virtual ~IDebugMenuElement();

    INLINE StringID action() const { return m_action; }
    INLINE bool enabled() const { return m_enabled; }

    struct RenderState
    {
        float displayScale = 1.0;
        Vector2 offset;
        Vector2 size;
        bool current = false;
        float timeSinceCurrent = 0.0f;
    };

    virtual void measure(IGameScreenDebugMenu* menu, Canvas& c, float displayScale, Vector2& outNeededSize) const = 0;
    virtual void render(IGameScreenDebugMenu* menu, Canvas& c, const RenderState& state) const = 0;
    virtual bool handleKey(IGameScreenDebugMenu* menu, InputKey key, bool repeat) = 0;

protected:
    StringID m_action;
    bool m_enabled = true;
};

//--

/// A simple debug option - button with callback
class GAME_COMMON_API DebugMenuButton : public IDebugMenuElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(DebugMenuButton, IDebugMenuElement);

public:
    DebugMenuButton(StringView caption, StringID action, bool enabled=true, int index=0);

    virtual void measure(IGameScreenDebugMenu* menu, Canvas& c, float displayScale, Vector2& outNeededSize) const override;
    virtual void render(IGameScreenDebugMenu* menu, Canvas& c, const RenderState& state) const override;
    virtual bool handleKey(IGameScreenDebugMenu* menu, InputKey key, bool repeat) override;

    virtual void clicked(IGameScreenDebugMenu* menu);

private:
    NativeTimePoint m_cooldown;
    StringBuf m_caption;
    int m_index = 0;
};

//--

/// A world-configured controlled for the game
class GAME_COMMON_API IGameScreenDebugMenu : public IGameScreenCanvas
{
    RTTI_DECLARE_VIRTUAL_CLASS(IGameScreenDebugMenu, IGameScreenCanvas);

public:
    IGameScreenDebugMenu(StringView title);
    virtual ~IGameScreenDebugMenu();

    //--

    void attachMenuElement(IDebugMenuElement* option);
    void detachMenuElement(IDebugMenuElement* option);

    //--

    void postAction(StringID action, int index=0);

    //--

    virtual void handleAction(StringID action, int index) = 0;

protected:
    virtual void handleAttached() override;
    virtual void handleUpdate(double dt) override;
    virtual bool handleInput(const InputEvent& evt) override;
    virtual void handleRender(Canvas& c, float visibility) override;

    virtual bool queryOpaqueState() const override;

    void navigate(int newIndex);

    //--

    StringBuf m_title;

    Array<RefPtr<IDebugMenuElement>> m_elements;
    int m_activeElement = -1;
    float m_activeElementTime = 0.0f;

    float m_offsetY = 0.0f;

    StringID m_postedAction;
    int m_postedActionIndex = 0;
};

//--

/// main debug menu
class GAME_COMMON_API GameScreenDebugMainMenu : public IGameScreenDebugMenu
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameScreenDebugMainMenu, IGameScreen);

public:
    GameScreenDebugMainMenu();

private:
    virtual void handleAction(StringID action, int index) override;
};

//--

/// settings debug menu
class GAME_COMMON_API GameScreenDebugSettingsMenu : public IGameScreenDebugMenu
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameScreenDebugSettingsMenu, IGameScreen);

public:
    GameScreenDebugSettingsMenu();

private:
    virtual void handleAction(StringID action, int index) override;
};

//--

/// in game pause menu
class GAME_COMMON_API GameScreenDebugInGamePause : public IGameScreenDebugMenu
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameScreenDebugInGamePause, IGameScreen);

public:
    GameScreenDebugInGamePause();

private:
    virtual void handleAction(StringID action, int index) override;
};

//--

/// debug menu to select scene to load
class GAME_COMMON_API GameScreenDebugLoadWorld : public IGameScreenDebugMenu
{
    RTTI_DECLARE_VIRTUAL_CLASS(GameScreenDebugLoadWorld, IGameScreen);

public:
    GameScreenDebugLoadWorld();

private:
    virtual void handleAction(StringID action, int index) override;

    Array<StringBuf> m_worldPaths;
};

//--

END_BOOMER_NAMESPACE()
