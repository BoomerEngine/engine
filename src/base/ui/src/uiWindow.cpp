/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\window #]
***/

#include "build.h"
#include "uiWindow.h"
#include "uiGeometryBuilder.h"
#include "uiElementStyle.h"
#include "uiElementHitCache.h"
#include "uiStyleValue.h"
#include "uiInputAction.h"
#include "uiImage.h"
#include "uiRenderer.h"
#include "uiTextLabel.h"
#include "uiButton.h"
#include "uiWindowTitlebar.h"

#include "base/canvas/include/canvas.h"
#include "uiElementConfig.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

RTTI_BEGIN_TYPE_NATIVE_CLASS(Window);
    RTTI_METADATA(ElementClassNameMetadata).name("Window");
RTTI_END_TYPE();

Window::Window(WindowFeatureFlags flags, base::StringView title)
    : m_title(title)
    , m_flags(flags)
{
    hitTest(HitTestState::Enabled);
    layoutVertical();

    enableAutoExpand(true, true);
    allowFocusFromClick(false);
    allowFocusFromKeyboard(false);
    allowHover(false);

    if (flags.test(WindowFeatureFlagBit::HasTitleBar))
        createChild<WindowTitleBar>(flags, title);
}

Window::~Window()
{
    delete m_requests;
    m_requests = nullptr;

    TRACE_INFO("Window '{}' destroyed", m_title);
}

WindowRequests* Window::createPendingRequests()
{
    if (!m_requests)
        m_requests = new WindowRequests;
    return m_requests;
}

WindowRequests* Window::consumePendingRequests()
{
    auto ret = m_requests;
    m_requests = nullptr;
    return ret;
}

bool Window::requestedClose() const
{
    return m_closeRequested;
}

void Window::requestClose(int exitCode)
{
    auto requests = createPendingRequests();
    if (!requests->requestClose)
    {
        TRACE_INFO("Requested to close window '{}'", this->name());
        requests->requestClose = true;
        call(EVENT_WINDOW_CLOSED);

        m_exitCode = exitCode;
        m_closeRequested = true;

        // hacky hack...
        if (auto owner = m_modalOwner.lock())
            owner->focus();
    }
}

void Window::requestActivate()
{
    auto requests = createPendingRequests();
    if (!requests->requestActivation)
        requests->requestActivation = true;
}

void Window::requestMaximize(bool restore)
{
    auto requests = createPendingRequests();
    if (!requests->requestMaximize)
        requests->requestMaximize = true;
}

void Window::requestMinimize()
{
    auto requests = createPendingRequests();
    if (!requests->requestMinimize)
        requests->requestMinimize = true;
}

void Window::requestMove(const Position& screenPosition)
{
    auto requests = createPendingRequests();
    if (!requests->requestPositionChange)
        requests->requestPositionChange = true;

    requests->requestedPosition = screenPosition;
}

void Window::requestSize(const Size& screenSize)
{
    auto requests = createPendingRequests();
    if (!requests->requestSizeChange)
        requests->requestSizeChange = true;

    requests->requestedSize = screenSize;
}

void Window::requestTitleChange(base::StringView newTitle)
{
    if (m_title != newTitle)
    {
        m_title = base::StringBuf(newTitle);

        auto requests = createPendingRequests();
        if (!requests->requestTitleChange)
        {
            TRACE_INFO("Requested to change window '{}' title to '{}'", this->name(), newTitle);
            requests->requestTitleChange = true;
        }

        requests->requestedTitle = base::StringBuf(newTitle);
    }
}

void Window::requestShow(bool activate)
{
    auto requests = createPendingRequests();
    if (activate)
        requests->requestActivation = activate;
    requests->requestShow = true;
    requests->requestHide = false;
}

void Window::requestHide()
{
    auto requests = createPendingRequests();
    requests->requestActivation = false;
    requests->requestShow = false;
    requests->requestHide = true;
}

void Window::prepare(DataStash& stash, float nativePixelScale, bool initial, bool forceUpdateExt)
{
    PC_SCOPE_LVL1(PrepareWindow);

    bool forceUpdate = (nativePixelScale != m_lastPixelScale) || initial || forceUpdateExt;
    m_lastPixelScale = nativePixelScale;

    {
        PC_SCOPE_LVL1(PrepareStyles);
        StyleStack stack(stash);
        prepareStyle(stack, nativePixelScale, forceUpdate);
    }

    // prepare layouts and geometry of all elements
    {
        PC_SCOPE_LVL1(PrepareLayout);
        bool layoutRecomputed = false;
        prepareLayout(layoutRecomputed, forceUpdate, initial);
    }
}

void Window::render(base::canvas::Canvas& canvas, HitCache& hitCache, DataStash& stash, const ElementArea& nativeArea)
{
    PC_SCOPE_LVL1(RenderWindow);

    // prepare clip
    canvas.scissorRect(nativeArea.absolutePosition(), nativeArea.size());

    // render into provided area
    IElement::render(hitCache, stash, nativeArea, nativeArea, canvas, 1.0f, nullptr);
}

void Window::handleExternalCloseRequest()
{
    requestClose();
}

void Window::renderBackground(DataStash& stash, const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
{
    TBaseClass::renderBackground(stash, drawArea, canvas, mergedOpacity);

    //if (mergedOpacity)

    if (m_clearColorHack)
        canvas.clearColor(m_clearColor);
}

void Window::prepareBackgroundGeometry(DataStash& stash, const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const
{
    if (auto shadowPtr = evalStyleValueIfPresentPtr<style::RenderStyle>("background"_id))
    {
        if (shadowPtr->type == 0)
        {
            m_clearColor = shadowPtr->innerColor;
            m_clearColorHack = true;
        }
        else
        {
            m_clearColorHack = false;
            TBaseClass::prepareBackgroundGeometry(stash, drawArea, pixelScale, builder);
        }
    }
}

Window* Window::findParentWindow() const
{
    return parentElement() ? parentElement()->findParentWindow() : const_cast<Window*>(this);
}

void Window::handleExternalActivation(bool isActive)
{
    if (isActive)
        addStyleClass("active"_id);
    else
        removeStyleClass("active"_id);
}

InputActionPtr Window::handleMouseClick(const ElementArea& area, const base::input::MouseClickEvent& evt)
{
    return nullptr;
}

int Window::QuerySizeCode(const ElementArea& area, const Position& absolutePosition)
{
    float minx = area.absolutePosition().x;
    float miny = area.absolutePosition().y;
    float maxx = minx + area.size().x;
    float maxy = miny + area.size().y;

    auto borderWidth = 6;

    bool onLeft = (abs(minx - absolutePosition.x) <= borderWidth);
    bool onRight = (abs(maxx - absolutePosition.x) <= borderWidth);
    bool onTop = (abs(miny - absolutePosition.y) <= borderWidth);
    bool onBottom = (abs(maxy - absolutePosition.y) <= borderWidth);

    if (onLeft || onRight || onTop || onBottom)
    {
        auto y = onBottom ? 2 : (onTop ? 0 : 1);
        auto x = onRight ? 2 : (onLeft ? 0 : 1);
        return x + (y * 3);
    }

    return -1;
}

bool Window::handleCursorQuery(const ElementArea& area, const Position& absolutePosition, base::input::CursorType& outCursorType) const
{
    if (queryResizableState())
    {
        auto code = QuerySizeCode(area, absolutePosition);
        if (code != -1 && code != 4)
        {
            base::input::CursorType areas[3][3] = {
                { base::input::CursorType::SizeNWSE, base::input::CursorType::SizeNS, base::input::CursorType::SizeNESW },
                { base::input::CursorType::SizeWE, base::input::CursorType::Default, base::input::CursorType::SizeWE },
                { base::input::CursorType::SizeNESW, base::input::CursorType::SizeNS, base::input::CursorType::SizeNWSE}
            };

            outCursorType = areas[code / 3][code % 3];
            return true;
        }
    }

    return TBaseClass::handleCursorQuery(area, absolutePosition, outCursorType);
}

bool Window::handleWindowFrameArea(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const
{
    // border stuff
    if (queryResizableState())
    {
        auto code = QuerySizeCode(area, absolutePosition);
        if (code != -1 && code != 4)
        {
            base::input::AreaType areas[3][3] = {
                { base::input::AreaType::BorderTopLeft , base::input::AreaType::BorderTop, base::input::AreaType::BorderTopRight },
                { base::input::AreaType::BorderLeft, base::input::AreaType::Caption, base::input::AreaType::BorderRight },
                { base::input::AreaType::BorderBottomLeft, base::input::AreaType::BorderBottom, base::input::AreaType::BorderBottomRight }
            };

            outAreaType = areas[code / 3][code % 3];
            return true;
        }
    }

    return false;
}

bool Window::handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const
{
    // allow to move whole window by dragging any non-active element
    if (queryMovableState())
    {
        outAreaType = base::input::AreaType::Caption;
        return true;
    }

    return false;
}

bool Window::queryCurrentPlacementForSaving(WindowSavedPlacementSetup& outPlacement) const
{
    if (auto* render = renderer())
        return render->queryWindowPlacement(this, outPlacement);

    return false;
}

void Window::queryInitialPlacementSetup(WindowInitialPlacementSetup& outSetup) const
{
    outSetup.title = m_title;
    outSetup.flagAllowResize = m_flags.test(WindowFeatureFlagBit::Resizable);
    outSetup.flagForceActive = !m_flags.test(WindowFeatureFlagBit::ShowInactive);
    outSetup.flagShowOnTaskBar = m_flags.test(WindowFeatureFlagBit::ShowOnTaskBar);
    outSetup.flagPopup = m_flags.test(WindowFeatureFlagBit::Popup);
    outSetup.flagTopMost = m_flags.test(WindowFeatureFlagBit::TopMost);
    outSetup.owner = base::rtti_cast<Window>(m_modalOwner.lock());
    outSetup.externalParentWindowHandle = m_parentModalWindowHandle;

    if (m_savedPlacementValid)
        outSetup.savedPlacement = &m_savedPlacement;

}

bool Window::queryMovableState() const
{
    if (auto* render = renderer())
        return render->queryWindowMovableState(this);

    return false;
}

bool Window::queryResizableState() const
{
    if (auto* render = renderer())
        return render->queryWindowResizableState(this);

    return false;
}

//--

RTTI_BEGIN_TYPE_STRUCT(WindowSavedPlacementSetup);
    RTTI_PROPERTY(rect);
    RTTI_PROPERTY(visible);
    RTTI_PROPERTY(maximized);
    RTTI_PROPERTY(minimized);
RTTI_END_TYPE();

void Window::configLoad(const ConfigBlock& block)
{
    TBaseClass::configLoad(block);

    WindowSavedPlacementSetup placement;
    if (block.read("placement", placement))
        applySavedPlacement(placement);
}

void Window::configSave(const ConfigBlock& block) const
{
    TBaseClass::configSave(block);

    WindowSavedPlacementSetup placement;
    if (queryCurrentPlacementForSaving(placement))
        block.write("placement", placement);
}

void Window::applySavedPlacement(const WindowSavedPlacementSetup& placement)
{
    if (auto render = renderer())
    {
        if (placement.maximized)
            requestMaximize();
        else if (placement.minimized)
            requestMinimize();
        else
        {
            requestMove(placement.rect.topLeft().toVector());
            requestSize(placement.rect.size().toVector());
        }

        if (placement.visible)
            requestShow(false);
        else
            requestHide();
    }
    else
    {
        m_savedPlacement = placement;
        m_savedPlacementValid = true;
    }
}

//--

int Window::runModal(IElement* owner, IElement* windowFocus)
{
    DEBUG_CHECK_RETURN_V(owner && owner->renderer(), 0);

    auto parentWindow = owner->findParentWindow();
    DEBUG_CHECK_RETURN_V(parentWindow, 0);

    m_exitCode = 0;
    m_modalOwner = parentWindow;
    m_parentModalWindowHandle = owner->renderer()->queryWindowNativeHandle(parentWindow);

    owner->renderer()->runModalLoop(this, windowFocus);
    return m_exitCode;
}

//--

END_BOOMER_NAMESPACE(ui)