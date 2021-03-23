/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"

#include "screen.h"
#include "screenStack.h"
#include "screenDebugMenu.h"
#include "screenSinglePlayerGame.h"

#include "platform.h"
#include "settings.h"

#include "core/resource/include/depot.h"
#include "core/input/include/inputStructures.h"
#include "core/app/include/projectSettingsService.h"
#include "core/math/include/lerp.h"

#include "engine/canvas/include/canvas.h"
#include "engine/canvas/include/geometryBuilder.h"
#include "engine/world/include/rawWorldData.h"
#include "engine/world/include/compiledWorldData.h"

#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE()

//---

ConfigProperty<float> cvDebugMenuTextSize("DebugMenu", "TextSize", 40.0f);
ConfigProperty<Color> cvDebugMenuNormalTextColor("DebugMenu", "NormalTextColor", Color::WHITE);
ConfigProperty<Color> cvDebugMenuNormalActiveColor("DebugMenu", "ActiveTextColor", Color::YELLOW);
ConfigProperty<Color> cvDebugMenuNormalDisabledColor("DebugMenu", "DisabledTextColor", Color::GREY);
ConfigProperty<float> cvDebugMenuBlinkSpeed("DebugMenu", "BlinkSpeed", 2.0f);
ConfigProperty<float> cvDebugMenuBlinkDepth("DebugMenu", "BlinkDepth", 0.3f);

ConfigProperty<float> cvDebugMenuHeaderTextSize("DebugMenu", "HeaderTextSize", 60.0f);
ConfigProperty<float> cvDebugMenuHeaderMarginX("DebugMenu", "HeaderMarginX", 80.0f);
ConfigProperty<float> cvDebugMenuHeaderMarginY("DebugMenu", "HeaderMarginY", 20.0f);

ConfigProperty<float> cvDebugMenuFadeInSpeed("DebugMenu", "FadeInSpeed", 5.0f);
ConfigProperty<float> cvDebugMenuFadeOutSpeed("DebugMenu", "FadeOutSpeed", 5.0f);

ConfigProperty<float> cvDebugMenuBackgroundDimming("DebugMenu", "BackgroundDimmin", 0.8f);

ConfigProperty<Color> cvDebugMenuActiveBackgroundColor("DebugMenu", "ActiveBackgroundColor", Color::BLUE);
ConfigProperty<Color> cvDebugMenuActiveDisabledBackgroundColor("DebugMenu", "ActiveDisabledBackgroundColor", Color::DARKGRAY);

ConfigProperty<float> cvDebugMenuItemPaddingX("DebugMenu", "ItemPaddingX", 30.0f);
ConfigProperty<float> cvDebugMenuItemPaddingY("DebugMenu", "ItemPaddingY", 10.0f);
ConfigProperty<float> cvDebugMenuItemMargin("DebugMenu", "ItemMargin", 4.0f);
ConfigProperty<float> cvDebugMenuItemMinWidth("DebugMenu", "ItemMinWidth", 200.0f);

ConfigProperty<float> cvDebugMenuPanelPadding("DebugMenu", "PanelPadding", 10.0f);
ConfigProperty<float> cvDebugMenuPanelMaxHeightPerc("DebugMenu", "PanelMaxHeightPerc", 0.8f);
ConfigProperty<float> cvDebugMenuPanelMaxWidthPerc("DebugMenu", "PanelMaxWidthPerc", 0.8f);
ConfigProperty<Color> cvDebugMenuPanelFillColor("DebugMenu", "PanelFillColor", Color(40,40,40,220));
ConfigProperty<Color> cvDebugMenuPanelTitleFillColor("DebugMenu", "PanelTitleFillColor", Color(60, 60, 60, 255));
ConfigProperty<Color> cvDebugMenuPanelTitleTextColor("DebugMenu", "PanelTitleTextColor", Color(255, 255, 255, 255));
ConfigProperty<Color> cvDebugMenuPanelBorderColor("DebugMenu", "PanelBorderColor", Color(100, 100, 100, 255));


//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IDebugMenuElement);
RTTI_END_TYPE();

IDebugMenuElement::IDebugMenuElement(StringID action)
    : m_action(action)
{
}

IDebugMenuElement::~IDebugMenuElement()
{}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(DebugMenuButton);
RTTI_END_TYPE();

DebugMenuButton::DebugMenuButton(StringView caption, StringID action, bool enabled, int index)
    : IDebugMenuElement(action)
    , m_caption(caption)
    , m_index(index)
{
    m_enabled = enabled;
}

void DebugMenuButton::measure(IGameScreenDebugMenu* menu, canvas::Canvas& c, float displayScale, Vector2& outNeededSize) const
{
    const auto fontSize = (int)(cvDebugMenuTextSize.get() * displayScale);
    auto bounds = c.debugPrintMeasure(m_caption, fontSize);
    auto line = c.debugPrintMeasure("Wag", fontSize);
    outNeededSize.x = bounds.size.x + (cvDebugMenuItemPaddingX.get() * 2.0f);
    outNeededSize.y = line.size.y + (cvDebugMenuItemPaddingY.get() * 2.0f);
}

void DebugMenuButton::render(IGameScreenDebugMenu* menu, canvas::Canvas& c, const RenderState& state) const
{
    // background
    if (state.current)
    {
        canvas::InplaceGeometryBuilder g(c);

        const auto color = m_enabled 
            ? cvDebugMenuActiveBackgroundColor.get()
            : cvDebugMenuActiveDisabledBackgroundColor.get();

        g.solidRect(color, state.offset.x, state.offset.y, state.size.x, state.size.y);
        g.render();
    }

    // caption
    {
        auto color = cvDebugMenuNormalTextColor.get();
        if (!m_enabled)
            color = cvDebugMenuNormalDisabledColor.get();
        else if (state.current)
            color = cvDebugMenuNormalActiveColor.get();

        const auto fontSize = (int)(cvDebugMenuTextSize.get() * state.displayScale);
        const auto bounds = c.debugPrintMeasure(m_caption, fontSize);

        float textX = state.offset.x + (state.size.x - bounds.size.x) * 0.5f;
        float textY = state.offset.y + (state.size.y - bounds.size.y) * 0.5f;
        c.debugPrint(textX, textY, m_caption, color, fontSize);
    }
}

bool DebugMenuButton::handleKey(IGameScreenDebugMenu* menu, input::KeyCode key, bool repeat)
{
    if (key == input::KeyCode::KEY_RETURN && !repeat)
    {
        if (!m_cooldown || m_cooldown.reached())
        {
            m_cooldown = NativeTimePoint::Now() + 0.2;

            clicked(menu);
            return true;
        }
    }

    return false;
}

void DebugMenuButton::clicked(IGameScreenDebugMenu* menu)
{
    if (m_enabled)
    {
        if (m_action)
            menu->postAction(m_action, m_index);
    }
}


//---

RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IGameScreenDebugMenu);
RTTI_END_TYPE();

IGameScreenDebugMenu::IGameScreenDebugMenu(StringView title)
    : m_title(title)
{
}

IGameScreenDebugMenu::~IGameScreenDebugMenu()
{}

void IGameScreenDebugMenu::attachMenuElement(IDebugMenuElement* option)
{
    m_elements.pushBack(AddRef(option));

    if (m_activeElement == -1)
        m_activeElement = 0;
}

void IGameScreenDebugMenu::detachMenuElement(IDebugMenuElement* option)
{
    m_elements.remove(option);
    m_activeElement = std::min<int>(m_activeElement, m_elements.lastValidIndex());
}

void IGameScreenDebugMenu::postAction(StringID action, int index)
{
    m_postedAction = action;
    m_postedActionIndex = index;
}

void IGameScreenDebugMenu::handleAttached()
{
    TBaseClass::handleAttached();
}

void IGameScreenDebugMenu::handleUpdate(double dt)
{
    if (auto action = m_postedAction)
    {
        int index = m_postedActionIndex;
        m_postedActionIndex = 0;
        m_postedAction = StringID();

        if (action == "Close"_id)
        {
            host()->pushTransition(nullptr, GameTransitionMode::ReplaceTop);
        }
        else
        {
            handleAction(action, index);
        }
    }
}

void IGameScreenDebugMenu::navigate(int newIndex)
{
    newIndex = std::clamp<int>(newIndex, 0, m_elements.lastValidIndex());

    if (newIndex != m_activeElement)
    {
        m_activeElement = newIndex;
        m_activeElementTime = 0.0f;
    }
}

bool IGameScreenDebugMenu::handleInput(const input::BaseEvent& evt)
{
    if (const auto* key = evt.toKeyEvent())
    {
        if (key->pressedOrRepeated())
        {
            switch (key->keyCode())
            {
                case input::KeyCode::KEY_UP:
                {
                    navigate(m_activeElement - 1);
                    return true;
                }

                case input::KeyCode::KEY_HOME:
                {
                    navigate(0);
                    return true;
                }

                case input::KeyCode::KEY_DOWN:
                {
                    navigate(m_activeElement + 1);
                    return true;
                }

                case input::KeyCode::KEY_END:
                {
                    navigate(m_elements.lastValidIndex());
                    return true;
                }

                case input::KeyCode::KEY_PRIOR:
                {
                    navigate(m_activeElement - 10);
                    return true;
                }

                case input::KeyCode::KEY_NEXT:
                {
                    navigate(m_activeElement + 10);
                    return true;
                }

                case input::KeyCode::KEY_ESCAPE:
                {
                    postAction("Close"_id);
                    return true;
                }
            }

            if (m_activeElement >= 0 && m_activeElement <= m_elements.lastValidIndex())
                return m_elements[m_activeElement]->handleKey(this, key->keyCode(), key->pressedOrRepeated() && !key->pressed());
        }
    }

    return false;
}

bool IGameScreenDebugMenu::queryOpaqueState() const
{
    return false;
}

void IGameScreenDebugMenu::handleRender(canvas::Canvas& c, float visibility)
{
    // display scale of the whole thing
    const auto displayScale = 1.0f;
    const auto itemMinWidth = cvDebugMenuItemMinWidth.get() * displayScale;
    const auto itemMargin = cvDebugMenuItemMargin.get() * displayScale;

    // measure all elements to render
    Vector2 itemsSize(10, 10);
    float activeElementMinY = 0.0f;
    float activeElementMaxY = 0.0f;
    InplaceArray<Vector2, 100> measuredSizes;
    for (auto i : m_elements.indexRange())
    {
        const auto& elem = m_elements[i];

        Vector2 elementSize(0, 0);
        elem->measure(this, c, displayScale, elementSize);

        if (elementSize.x < itemMinWidth)
            elementSize.x = itemMinWidth;

        if (elementSize.x > itemsSize.x)
            itemsSize.x = elementSize.x;

        if (i == m_activeElement)
        {
            activeElementMinY = itemsSize.y - itemMargin * 0.5f;
            activeElementMaxY = itemsSize.y + elementSize.y * itemMargin * 0.5f;
        }

        itemsSize.y += elementSize.y + itemMargin;

        measuredSizes.pushBack(elementSize);
    }

    // padd the item area
    itemsSize.x += cvDebugMenuPanelPadding.get() * 2.0f;
    itemsSize.y += cvDebugMenuPanelPadding.get() * 2.0f;

    // measure header
    Vector2 headerSize(0,0), headerTextSize(0,0);
    const auto headerFontSize = cvDebugMenuHeaderTextSize.get() * displayScale;
    const auto headerMarginX = cvDebugMenuHeaderMarginX.get() * displayScale;
    const auto headerMarginY = cvDebugMenuHeaderMarginY.get() * displayScale;
    if (!m_title.empty())
    {
        const auto bounds = c.debugPrintMeasure(m_title, (int)headerFontSize, font::FontAlignmentHorizontal::Center, true);
        headerTextSize = bounds.size;
        headerSize = bounds.size;
        headerSize.x += headerMarginX * 2.0f;
        headerSize.y += headerMarginY * 2.0f;
    }

    // calculate total size of the menu
    Vector2 totalSize(0,0);
    totalSize.x = std::max<float>(itemsSize.x, headerSize.x);
    totalSize.y = itemsSize.y + headerSize.y;

    // limit the total size to some % of the screen
    const auto maxTotalWidth = cvDebugMenuPanelMaxWidthPerc.get() * c.width();
    const auto maxTotalHeight = cvDebugMenuPanelMaxHeightPerc.get() * c.height();
    totalSize.x = std::min<float>(totalSize.x, maxTotalWidth);
    totalSize.y = std::min<float>(totalSize.y, maxTotalHeight);

    // center 
    const auto offsetX = (c.width() - totalSize.x) * 0.5f;
    const auto offsetY = (c.height() - maxTotalHeight) * 0.5f + ((visibility - 1.0f) * c.height());

    // draw the background
    {
        float backgroundAlpha = LinearInterpolation(visibility).lerp(1.0f, cvDebugMenuBackgroundDimming.get());
        if (backgroundAlpha < 1.0f)
        {
            Color color;
            color.r = color.g = color.b = 0;
            color.a = (uint8_t)(backgroundAlpha * 255.0f);

            canvas::InplaceGeometryBuilder g(c);
            g.solidRect(color, 0, 0, c.width(), c.height());
            g.render();
        }
    }

    // draw the frame
    {
        canvas::InplaceGeometryBuilder g(c);
        g.solidRectWithBorder(cvDebugMenuPanelFillColor.get(), cvDebugMenuPanelBorderColor.get(), offsetX, offsetY, totalSize.x, totalSize.y);

        if (!m_title.empty())
            g.solidRectWithBorder(cvDebugMenuPanelTitleFillColor.get(), cvDebugMenuPanelBorderColor.get(), offsetX, offsetY, totalSize.x, headerSize.y);

        g.render();
    }

    // draw the title
    if (!m_title.empty())
    {
        auto titleOffsetX = offsetX + (totalSize.x - headerTextSize.x) * 0.5f;
        auto titleOffsetY = offsetY + (headerSize.y - headerTextSize.y) * 0.5f;
        c.debugPrint(titleOffsetX, titleOffsetY, m_title, cvDebugMenuPanelTitleTextColor.get(), (int)headerFontSize);

    }

    // item scroll offset
    auto scrollOffsetY = cvDebugMenuPanelPadding.get() + headerSize.y;

    // draw items
    {
        c.pushScissorRect();
        c.scissorRect(offsetX, offsetY + headerSize.y, totalSize.x, itemsSize.y);

        for (auto i : m_elements.indexRange())
        {
            IDebugMenuElement::RenderState state;
            state.current = (i == m_activeElement);
            state.timeSinceCurrent = m_activeElementTime;
            state.displayScale = displayScale;

            const auto localOffsetX = std::max<float>(0.0f, itemsSize.x - measuredSizes[i].x) * 0.5f;
            state.offset.x = offsetX + cvDebugMenuPanelPadding.get();
            state.offset.y = offsetY + scrollOffsetY;
            state.size.x = totalSize.x - 2.0f * cvDebugMenuPanelPadding.get();
            state.size.y = measuredSizes[i].y;

            scrollOffsetY += measuredSizes[i].y + itemMargin;

            m_elements[i]->render(this, c, state);
        }

        c.popScissorRect();
    }
}

//---

RTTI_BEGIN_TYPE_CLASS(GameScreenDebugMainMenu);
RTTI_END_TYPE();

GameScreenDebugMainMenu::GameScreenDebugMainMenu()
    : IGameScreenDebugMenu("")
{
    if (IsDefaultObjectCreation())
        return;

    {
        auto option = RefNew<DebugMenuButton>("Start game", "StartGame"_id, false);
        attachMenuElement(option);
    }

    const auto settings = GetSettings<GameProjectSettingScreens>();
    if (settings->m_singlePlayerGameScreen)
    {
        {
            auto option = RefNew<DebugMenuButton>("Load world", "LoadWorld"_id, true);
            attachMenuElement(option);
        }
    }

    {
        auto option = RefNew<DebugMenuButton>("Settings", "Settings"_id, true);
        attachMenuElement(option);
    }

    {
        auto option = RefNew<DebugMenuButton>("Exit to desktop", "Close"_id, true);
        attachMenuElement(option);
    }
}

void GameScreenDebugMainMenu::handleAction(StringID action, int index)
{
    if (action == "ExitToDesktop"_id || action == "Close"_id)
    {
        host()->requestExit();
    }
    else if (action == "LoadWorld"_id)
    {
        auto screen = RefNew<GameScreenDebugLoadWorld>();
        host()->pushTransition(screen, GameTransitionMode::PushOnTop);
    }
    else if (action == "Settings"_id)
    {
        auto screen = RefNew<GameScreenDebugSettingsMenu>();
        host()->pushTransition(screen, GameTransitionMode::PushOnTop);
    }
}

//---

RTTI_BEGIN_TYPE_CLASS(GameScreenDebugSettingsMenu);
RTTI_END_TYPE();

GameScreenDebugSettingsMenu::GameScreenDebugSettingsMenu()
    : IGameScreenDebugMenu("Settings")
{
    if (IsDefaultObjectCreation())
        return;

    {
        auto option = RefNew<DebugMenuButton>("Graphics", "Graphics"_id);
        attachMenuElement(option);
    }

    {
        auto option = RefNew<DebugMenuButton>("Video", "Video"_id);
        attachMenuElement(option);
    }

    {
        auto option = RefNew<DebugMenuButton>("Audio", "Audio"_id);
        attachMenuElement(option);
    }

    {
        auto option = RefNew<DebugMenuButton>("Controls", "Controls"_id);
        attachMenuElement(option);
    }

    {
        auto option = RefNew<DebugMenuButton>("Game", "Game"_id);
        attachMenuElement(option);
    }

    {
        auto option = RefNew<DebugMenuButton>("Back", "Close"_id);
        attachMenuElement(option);
    }
}

void GameScreenDebugSettingsMenu::handleAction(StringID action, int index)
{
    if (action == "Close"_id)
    {
        host()->pushTransition(nullptr, boomer::GameTransitionMode::ReplaceTop);
    }
}

//---

RTTI_BEGIN_TYPE_CLASS(GameScreenDebugInGamePause);
RTTI_END_TYPE();

GameScreenDebugInGamePause::GameScreenDebugInGamePause()
    : IGameScreenDebugMenu("Paused")
{
    if (IsDefaultObjectCreation())
        return;

    {
        auto option = RefNew<DebugMenuButton>("Continue", "Close"_id);
        attachMenuElement(option);
    }

    {
        auto option = RefNew<DebugMenuButton>("Exit to Menu", "ExitToMenu"_id);
        attachMenuElement(option);
    }

    {
        auto option = RefNew<DebugMenuButton>("Quit to Desktop", "ExitToDesktop"_id);
        attachMenuElement(option);
    }
}

void GameScreenDebugInGamePause::handleAction(StringID action, int index)
{
    if (action == "Close"_id)
    {
        host()->pushTransition(nullptr, boomer::GameTransitionMode::ReplaceTop);
    }
    else if (action == "ExitToMenu"_id)
    {
        auto mainMenu = GetSettings<GameProjectSettingScreens>()->m_mainMenuScreen->create<IGameScreen>();
        host()->pushTransition(mainMenu, GameTransitionMode::ReplaceAll);
    }
    else if (action == "ExitToDesktop"_id)
    {
        host()->requestExit();
    }
}

//---

RTTI_BEGIN_TYPE_CLASS(GameScreenDebugLoadWorld);
RTTI_END_TYPE();

static void CollectRawWorldPaths(Array<StringBuf>& outDepotPaths)
{
    HashSet<StringBuf> paths;

    const auto& settings = GetAllSettings<GameProjectSettingDebugWorlds>();
    for (const auto& set : settings)
    {
        for (const auto& id : set->m_worlds)
        {
            StringBuf worldPath;
            if (GetService<DepotService>()->resolvePathForID(id.id(), worldPath))
            {
                paths.insert(worldPath);
            }
        }

        for (const auto& path : set->m_worldPaths)
        {
            if (CanLoadAsClass<RawWorldData>(path))
            {
                paths.insert(path);
            }
            else
            {
                TRACE_WARNING("World path '{}' specifieds a file that can't be loaded as a world", path);
            }
        }
    }

    outDepotPaths = paths.keys();

    std::sort(outDepotPaths.begin(), outDepotPaths.end(), [](const auto& a, const auto& b)
        {
            return a < b;
        });
}

GameScreenDebugLoadWorld::GameScreenDebugLoadWorld()
    : IGameScreenDebugMenu("Load World")
{
    if (IsDefaultObjectCreation())
        return;

    CollectRawWorldPaths(m_worldPaths);

    for (auto i : m_worldPaths.indexRange())
    {
        const auto& path = m_worldPaths[i];

        auto option = RefNew<DebugMenuButton>(path, "LoadWorld"_id, true, i);
        attachMenuElement(option);
    }

    if (m_worldPaths.empty())
    {
        auto option = RefNew<DebugMenuButton>("(no world)", ""_id, false);
        attachMenuElement(option);
    }

    {
        auto option = RefNew<DebugMenuButton>("Back", "Close"_id);
        attachMenuElement(option);
    }
}

void GameScreenDebugLoadWorld::handleAction(StringID action, int index)
{
    if (action == "LoadWorld"_id)
    {
        const auto settings = GetSettings<GameProjectSettingScreens>();
        if (settings->m_singlePlayerGameScreen)
        {
            if (auto screen = settings->m_singlePlayerGameScreen->create<IGameScreenSinglePlayer>())
            {
                GameScreenSinglePlayerSetup setup;
                setup.localPlayerIdentity = host()->platform()->queryLocalPlayer(0);
                setup.worldPath = m_worldPaths[index];
                if (screen->initialize(setup))
                {
                    host()->pushTransition(screen, GameTransitionMode::ReplaceAll, true);
                }
            }
        }
    }
}

//---

END_BOOMER_NAMESPACE()
