/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: host #]
***/

#include "build.h"
#include "uiDataStash.h"
#include "uiRenderer.h"
#include "uiWindow.h"
#include "uiWindowPopup.h"
#include "uiInputAction.h"
#include "uiElementHitCache.h"
#include "uiDragDrop.h"
#include "engine/canvas/include/canvas.h"
#include "core/system/include/thread.h"
#include "core/app/include/localServiceContainer.h"
#include "core/containers/include/stringVector.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

ConfigProperty<float> cvHoverDuration("UI", "HoverDurationMS", 200.0f);

//---

struct ElementChildToParentIterator
{
    INLINE ElementChildToParentIterator(const ElementWeakPtr& element)
        : m_element(element.lock())
    {
    }

    INLINE ElementChildToParentIterator(const ElementPtr& element)
        : m_element(element)
    {
    }

    INLINE ElementChildToParentIterator(IElement* element)
        : m_element(element)
    {}

    INLINE operator bool() const
    {
        return m_element != nullptr;
    }

    INLINE void operator++()
    {
        m_element = (m_element && m_element->parentElement()) ? m_element->parentElement() : nullptr;
    }

    INLINE IElement* operator->() const
    {
        return m_element;
    }

    INLINE IElement* operator*() const
    {
        return m_element;
    }

    INLINE IElement* ptr() const
    {
        return m_element;
    }

private:
    IElement* m_element;
};

//---

struct ElementParentToChildIterator
{
    INLINE ElementParentToChildIterator(const ElementWeakPtr& element)
    {
        collect(element.lock());
    }

    INLINE ElementParentToChildIterator(const ElementPtr& element)
    {
        collect(element);
    }

    INLINE ElementParentToChildIterator(IElement* element)
    {
        if (element)
            collect(element);
    }        

    INLINE operator bool() const
    {
        return m_index < m_elements.size();
    }

    INLINE void operator++()
    {
        m_index += 1;
    }

    INLINE IElement* operator->() const
    {
        return (m_index < m_elements.size()) ? m_elements[m_index] : nullptr;
    }

    INLINE IElement* operator*() const
    {
        return (m_index < m_elements.size()) ? m_elements[m_index] : nullptr;
    }

    INLINE IElement* ptr() const
    {
        return (m_index < m_elements.size()) ? m_elements[m_index] : nullptr;
    }

private:
    InplaceArray<IElement*, 64> m_elements;
    uint16_t m_index = 0;

    void collect(IElement* ptr)
    {
        while (ptr)
        {
            m_elements.pushBack(ptr);
            ptr = ptr->parentElement() ? ptr->parentElement() : nullptr;
        }

        std::reverse(m_elements.begin(), m_elements.end());
    }
};

//---

INativeWindowCallback::~INativeWindowCallback()
{}

//---

IRendererNative::~IRendererNative()
{}

//---

Renderer::Renderer(DataStash* dataStash, IRendererNative* nativeRenderer)
    : m_stash(dataStash)
    , m_native(nativeRenderer)
{}

Renderer::~Renderer()
{
    auto windows = m_windowList;
    for (const auto& wnd : windows)
        dettachWindow(wnd);

    for (auto& window : m_windows)
    {
        if (window.nativeId)
        {
            m_native->windowDestroy(window.nativeId);
            window.nativeId = 0;
        }

        window.window->bindNativeWindowRenderer(nullptr);
    }
}

void Renderer::attachWindow(Window* window, IElement* focusElement)
{
    if (window)
    {
        DEBUG_CHECK_EX(!window->parentElement(), "Windows cannot be parented to anything else");
        if (!window->parentElement())
        {
            auto ptr = RefPtr<Window>(AddRef(window));

            m_windowList.pushBackUnique(ptr);
            window->bindNativeWindowRenderer(this);

            auto& entry = m_windows.emplaceBack();
            entry.window = ptr;
            entry.autoFocus = focusElement;
        }
    }
}

void Renderer::dettachWindow(Window* window)
{
    if (m_windowList.remove(window))
    {
        // request window to be closed
        for (auto& entry : m_windows)
        {
            if (entry.window == window)
            {
                entry.closeAndRemove = true;
                break;
            }
        }
    }
}

void Renderer::prepareForModalLoop()
{
    // disable all windows
    for (auto& entry : m_windows)
        if (entry.nativeId)
            m_native->windowEnable(entry.nativeId, false);
}

void Renderer::returnFromModalLoop()
{
    // reenable all windows
    for (auto& entry : m_windows)
        if (entry.nativeId)
            m_native->windowEnable(entry.nativeId, true);

}

void Renderer::runModalLoop(Window* window, IElement* focusElement)
{
    DEBUG_CHECK_EX(window, "No window to run modal loop for");

    prepareForModalLoop();

    {
        Renderer childRenderer(m_stash, m_native);
        childRenderer.attachWindow(window, focusElement);

        // loop until we request the window to close
        NativeTimePoint timeDelta;
        while (childRenderer.windows().contains(window) && !window->requestedClose())
        {
            ServiceContainer::GetInstance().update();

            auto dt = timeDelta.timeTillNow().toSeconds();
            timeDelta.resetToNow();

            childRenderer.updateAndRender(dt);
        }

        // note: it's important to Enable back the previous windows before destroying the renderer
        returnFromModalLoop();
    }
}

void Renderer::attachTimer(IElement* tickable, Timer* timer)
{
    DEBUG_CHECK_EX(!m_attachedTimers.contains(timer), "Timer is already attached");
    DEBUG_CHECK_EX(timer->scheduled(), "Attached timer should be scheduled");
    if (timer->scheduled() && !m_attachedTimers.contains(timer))
    {
        auto* entry = m_timerEntryPool.create();
        entry->timer = timer;
        entry->callTime = timer->nextTick();
        entry->owner = tickable;
        m_attachedTimers[timer] = entry;

        if (m_timerListTail) // timers usually happen in the future, start visiting the list from the end, TODO: improve
        {
            auto linkAfter = m_timerListTail;
            while (linkAfter)
            {
                if (entry->callTime >= linkAfter->callTime)
                    break;
                linkAfter = linkAfter->prev;
            }

            if (linkAfter == nullptr)
            {
                m_timerListHead->prev = entry;
                entry->next = m_timerListHead;
                m_timerListHead = entry;
            }
            else if (linkAfter->next == nullptr)
            {
                ASSERT(m_timerListTail == linkAfter);
                linkAfter->next = entry;
                entry->prev = linkAfter;
                m_timerListTail = entry;
            }
            else
            {
                ASSERT(m_timerListTail != linkAfter);
                auto nextNext = linkAfter->next;
                linkAfter->next = entry;
                nextNext->prev = entry;
                entry->prev = linkAfter;
                entry->next = nextNext;
            }
        }
        else
        {
            m_timerListTail = entry;
            m_timerListHead = entry;
        }
    }
}

void Renderer::detachTimer(IElement* tickable, Timer* timer)
{
    DEBUG_CHECK_EX(m_attachedTimers.contains(timer), "Timer is not attached");
    TimerEntry* entry = nullptr;
    if (m_attachedTimers.remove(timer, &entry))
    {
        // fixup iteration
        if (entry == m_timerIterationNext)
            m_timerIterationNext = entry->next;

        // unlink from list
        if (entry->next)
        {
            DEBUG_CHECK(m_timerListTail != entry);
            entry->next->prev = entry->prev;
        }
        else
        {
            DEBUG_CHECK(m_timerListTail == entry);
            m_timerListTail = entry->prev;
        }

        if (entry->prev)
        {
            DEBUG_CHECK(m_timerListHead != entry);
            entry->prev->next = entry->next;
        }
        else
        {
            DEBUG_CHECK(m_timerListHead == entry);
            m_timerListHead = entry->next;
        }

        // return to pool
        m_timerEntryPool.free(entry);
    }
}

void Renderer::requestFocus(IElement* ptr, bool activateWindow /*= true*/)
{
    m_requestFocusElement = ptr;
    m_requestFocusElementSet = true;

    // TODO: maybe delay this 
    if (ptr && activateWindow)
    {
        if (auto window = ptr->findParentWindow())
            window->requestActivate();
    }
}

Size Renderer::calcWindowContentSize(Window* window, float pixelScale)
{
    window->prepare(*m_stash, pixelScale, true);
    return window->cachedLayoutParams().calcTotalSize();
}

Position Renderer::calcWindowPlacement(Size& size, const WindowInitialPlacementSetup& placement)
{
    if (placement.size.x > 0.0f)
        size.x = placement.size.x;
    if (placement.size.y > 0.0f)
        size.y = placement.size.y;

    TRACE_INFO("Initial size for '{}': [{}x{}]", placement.title, size.x, size.y);

    if (size.x < 16)
        size.x = 16;
    if (size.y < 16)
        size.y = 16;

    Window* referenceWindow = nullptr;
    ElementArea referenceArea(m_native->mousePosition(), Size(0, 0));
    if (!placement.flagRelativeToCursor)
    {
        if (auto referenceElement = placement.referenceElement.lock())
        {
            if (!referenceElement->cachedDrawArea().empty())
                referenceArea = referenceElement->cachedDrawArea();

            referenceWindow = referenceElement->findParentWindow();
            if (referenceWindow->cachedDrawArea().empty())
                referenceWindow = nullptr; // do not use invalid window reference area
        }
    }

    TRACE_INFO("Reference position for '{}': [{}x{}] [{}x{}]", placement.title, referenceArea.absolutePosition().x, referenceArea.absolutePosition().y, referenceArea.size().x, referenceArea.size().y);

    auto screenArea = m_native->windowMonitorAtPos(referenceArea.absolutePosition());

    if (size.x > screenArea.size().x)
        size.x = screenArea.size().x;
    if (size.y > screenArea.size().y)
        size.y = screenArea.size().y;

    bool limitToScreenArea = true;
    Position position = referenceArea.absolutePosition();
    if (placement.mode == WindowInitialPlacementMode::ScreenCenter || placement.mode == WindowInitialPlacementMode::Maximize || (placement.mode == WindowInitialPlacementMode::WindowCenter && !referenceWindow))
    {
        position.x = screenArea.absolutePosition().x + (screenArea.size().x - size.x) / 2.0f;
        position.y = screenArea.absolutePosition().y + (screenArea.size().y - size.y) / 2.0f;
        limitToScreenArea = false;
    }
    else if (placement.mode == WindowInitialPlacementMode::AreaCenter)
    {
        position.x = referenceArea.center().x - (size.x / 2.0f);
        position.y = referenceArea.center().y - (size.x / 2.0f);
        limitToScreenArea = true;
    }
    else if (placement.mode == WindowInitialPlacementMode::WindowCenter)
    {
        auto windowArea = referenceWindow->cachedDrawArea();

        position.x = windowArea.absolutePosition().x + (windowArea.size().x - size.x) / 2.0f;
        position.y = windowArea.absolutePosition().y + (windowArea.size().y - size.y) / 2.0f;
        limitToScreenArea = false;
    }
    else if (placement.mode == WindowInitialPlacementMode::BottomLeft)
    {
        position.x = referenceArea.absolutePosition().x;
        position.y = referenceArea.absolutePosition().y + referenceArea.size().y;
    }
    else if (placement.mode == WindowInitialPlacementMode::BottomRight)
    {
        position.x = referenceArea.absolutePosition().x + referenceArea.size().x - size.x;
        position.y = referenceArea.absolutePosition().y + referenceArea.size().y;
    }
    else if (placement.mode == WindowInitialPlacementMode::TopLeft)
    {
        position.x = referenceArea.absolutePosition().x;
        position.y = referenceArea.absolutePosition().y - size.y;
    }
    else if (placement.mode == WindowInitialPlacementMode::TopRight)
    {
        position.x = referenceArea.absolutePosition().x + referenceArea.size().x - size.x;
        position.y = referenceArea.absolutePosition().y - size.y;
    }
    else if (placement.mode == WindowInitialPlacementMode::TopRightNeighbour)
    {
        position.x = referenceArea.absolutePosition().x + referenceArea.size().x;
        position.y = referenceArea.absolutePosition().y;
    }        
    else
    {
        DEBUG_CHECK(!"Invalid placement mode");
    }

    position.x += placement.offset.x;
    position.y += placement.offset.y;

    if (limitToScreenArea)
    {
        if (position.x + size.x > screenArea.right())
            position.x = screenArea.right() - size.x;
        if (position.y + size.y > screenArea.bottom())
            position.y = screenArea.bottom() - size.y;

        if (position.x < screenArea.left())
            position.x = screenArea.left();
        if (position.y < screenArea.top())
            position.y = screenArea.top();
    }

    TRACE_INFO("Placement position for '{}': [{}x{}]", placement.title, position.x, position.y);

    return position;
}

static bool IsValidSavedPlacement(const Rect& placement)
{
    const auto size = placement.size();
    return size.x >= 100 && size.y >= 100;
}

void Renderer::updateWindowRepresentation(WindowInfo& window)
{
    // check for external close requests
    if (window.nativeId && m_native->windowHasCloseRequest(window.nativeId))
        window.window->handleExternalCloseRequest();

    // window can't be parented to anything
    DEBUG_CHECK_EX(window.window->parentElement() == nullptr, "Windows cannot be children of any other UI elements");
    if (window.window->parentElement() != nullptr)
        window.closeAndRemove = true; // close such window

    // we requested to close this window
    if (window.closeAndRemove)
    {
		// remove renderer
		window.window->bindNativeWindowRenderer(nullptr);

        // if we had native representation for it destroy it
        if (window.nativeId)
        {
            TRACE_WARNING("Destroying window native ID {}", window.nativeId);
            m_native->windowDestroy(window.nativeId);
            window.nativeId = 0;
        }

        return;
    }

    // window is renderable, create visual representation for it
    if (window.nativeId == 0)
    {
        WindowInitialPlacementSetup placement;
        window.window->queryInitialPlacementSetup(placement);

        auto pixelScale = 1.0f; // TODO: better guess

        NativeWindowSetup setup;
        setup.maximized = (placement.mode == WindowInitialPlacementMode::Maximize);
        setup.minimized = false;
        setup.title = placement.title;
        setup.topmost = placement.flagTopMost;
        setup.popup = placement.flagPopup;
        setup.showOnTaskBar = placement.flagShowOnTaskBar;
        setup.activate = placement.flagForceActive;
        setup.callback = this;
        setup.visible = true;

        bool hadAutomaticSizing = false;
        if (placement.savedPlacement && IsValidSavedPlacement(placement.savedPlacement->rect))
        {
            setup.position = placement.savedPlacement->rect.min;
            setup.size = placement.savedPlacement->rect.size();

            /*if (!placement.savedPlacement->visible)
                setup.visible = false;*/
        }
        else
        {
            hadAutomaticSizing = true;

            auto size = calcWindowContentSize(window.window.get(), pixelScale);
            setup.position = calcWindowPlacement(size, placement);
            setup.size = size;
        }

        if (placement.externalParentWindowHandle)
        {
            setup.externalParentWindowHandle = placement.externalParentWindowHandle;
        }
        else if (auto owner = placement.owner.lock())
        {
            for (const auto& otherInfo : m_windows)
            {
                if (otherInfo.window == owner)
                {
                    setup.owner = otherInfo.nativeId;
                    break;
                }
            }
        }

        window.resizable = placement.flagAllowResize;
        window.popup = placement.flagPopup;
            
        window.nativeId = m_native->windowCreate(setup);
        DEBUG_CHECK_EX(window.nativeId != 0, "Unable to create native window, we will retry next frame");
        if (window.nativeId)
        {
            if (hadAutomaticSizing)
            {
                float windowPixelScale = m_native->windowPixelScale(window.nativeId);
                if (windowPixelScale != pixelScale)
                {
                    auto adjustedSize = calcWindowContentSize(window.window.get(), windowPixelScale);
                    auto adjustedPosition = calcWindowPlacement(adjustedSize, placement);
                    m_native->windowSetPlacement(window.nativeId, adjustedPosition, adjustedSize);
                }
            }

            auto first = window.autoFocus.unsafe();

            if (!first)
                first = window.window->focusFindFirst();

            while (first)
            {
                auto deeper = first->focusFindFirst();
                if (!deeper || deeper == first)
                    break;
                first = deeper;
            }

            if (first)
                focus(first);
        }
    }
}

void Renderer::updateWindowState(WindowInfo& window)
{
    // service incoming request
    UniquePtr<WindowRequests> requests(window.window->consumePendingRequests());
    if (requests)
    {
        if (requests->requestClose)
        {
            TRACE_WARNING("Requested window {} to close", window.nativeId);
            window.closeAndRemove = true;
        }

        if (requests->requestActivation)
            m_native->windowSetFocus(window.nativeId);

        if (requests->requestHide)
            m_native->windowShow(window.nativeId, false);

        if (requests->requestShow)
            m_native->windowShow(window.nativeId, true);

        if (requests->requestMaximize)
            m_native->windowMaximize(window.nativeId);

        if (requests->requestMinimize)
            m_native->windowMinimize(window.nativeId);

        if (requests->requestPositionChange && requests->requestSizeChange)
            m_native->windowSetPlacement(window.nativeId, requests->requestedPosition, requests->requestedSize);
        else if (requests->requestPositionChange)
            m_native->windowSetPos(window.nativeId, requests->requestedPosition);
        else if (requests->requestSizeChange)
            m_native->windowSetSize(window.nativeId, requests->requestedSize);

        if (requests->requestTitleChange)
            m_native->windowSetTitle(window.nativeId, requests->requestedTitle);

        m_native->windowUpdate(window.nativeId);
    }
}

void Renderer::focus(IElement* element)
{
    m_requestFocusElement.reset();
    m_requestFocusElementSet = false;

    if (m_currentFocusElement != element)
    {
        if (auto current = m_currentFocusElement.lock())
        {
            TRACE_INFO("Focus lost on '{}' '{}'", current->name(), current->cls()->name());
            current->handleFocusLost();
        }

        m_currentFocusElement = element;
        if (auto next = m_currentFocusElement.lock())
        {
            TRACE_INFO("Focus gained on '{}' '{}'", next->name(), next->cls()->name());
            next->handleFocusGained();
        }
    }

    m_currentFocusElementStack.reset();
    while (element)
    {
        m_currentFocusElementStack.pushBack(element);
        element = element->parentElement();
    }

    //std::reverse(m_currentFocusElementStack.begin(), m_currentFocusElementStack.end());
}
    
void Renderer::updateFocusRequest()
{
    if (m_requestFocusElementSet)
    {
        m_requestFocusElementSet = false;
        m_tooltipFrozenUntilNextMouseMove = true;
        focus(m_requestFocusElement.lock().get());
    }

    if (!m_currentFocusElementStack.empty())
    {
        IElement* fallbackFocusElement = nullptr;
        for (const auto& elem : m_currentFocusElementStack)
        {
            if (auto current = elem.lock())
            {
                fallbackFocusElement = current;
                break;
            }
        }

        if (m_currentFocusElement != fallbackFocusElement)
            focus(fallbackFocusElement);
    }
}

void Renderer::updateHoverPosition()
{
    if (!m_currentInputAction || m_currentInputAction->allowsHoverTracking())
    {
        const auto absolutePosition = m_native->mousePosition();
        if (absolutePosition != m_lastHoverUpdatePosition)
        {
            m_lastHoverUpdateTime = NativeTimePoint::Now();
            m_tooltipFrozenUntilNextMouseMove = false;
        }

        m_lastHoverUpdatePosition = absolutePosition;

        // get the window at given position
        ElementPtr newHoverElement;
        if (auto windowInfo = windowAtPos(m_lastHoverUpdatePosition))
        {
            // only modal windows can target hover actions
            if (isWindowInteractionAllowed(windowInfo))
                newHoverElement = windowInfo->hitCache->traceElement(m_lastHoverUpdatePosition);
        }

        // change the active element
        // TODO: expose "hover delay" for some elements to reduce visual noise
        if (m_currentHoverElement != newHoverElement)
        {
            if (auto elem = m_currentHoverElement.lock())
                elem->handleHoverLeave(m_lastHoverUpdatePosition);

            m_currentHoverElement = newHoverElement;

            if (auto elem = m_currentHoverElement.lock())
            {
                //TRACE_INFO("Hover: '{}' ({})", elem->name(), elem->cls()->name());
                elem->handleHoverEnter(m_lastHoverUpdatePosition);
            }
        }            
    }
}

bool Renderer::isWindowInteractionAllowed(WindowInfo* window) const
{
    if (!window->hitCache)
        return false;
    return true;
}

Renderer::WindowInfo* Renderer::windowAtPos(Position absolutePosition)
{
    if (auto nativeWindowId = m_native->windowAtPos(m_lastHoverUpdatePosition))
        for (auto& info : m_windows)
            if (info.nativeId == nativeWindowId)
                return &info;

    return nullptr;
}

void Renderer::cancelTooltip()
{
    resetTooltip();
}

void Renderer::cancelCurrentInputAction()
{
    if (m_currentInputAction)
    {
        if (auto prevCaptureWindowId = nativeWindowForElement(m_currentInputAction->element().get()))
            m_native->windowSetCapture(prevCaptureWindowId, 0);

        m_currentInputAction->onCanceled();
        m_currentInputAction.reset();
    }
}

Renderer::WindowInfo* Renderer::windowForElement(IElement* element)
{
    if (element)
    {
        if (auto window = element->findParentWindow())
        {
            for (auto& info : m_windows)
                if (info.window == window)
                    return &info;
        }
    }

    return nullptr;
}

NativeWindowID Renderer::nativeWindowForElement(IElement* element)
{
    if (element)
    {
        if (auto window = element->findParentWindow())
        {
            for (auto& info : m_windows)
                if (info.window == window)
                    return info.nativeId;
        }
    }

    return (NativeWindowID)0;
}

bool Renderer::queryWindowResizableState(const Window* window) const
{
    for (auto& info : m_windows)
        if (info.window == window)
            return info.resizable && m_native->windowGetResizable(info.nativeId);

    return false;
}

bool Renderer::queryWindowMovableState(const Window* window) const
{
    for (auto& info : m_windows)
        if (info.window == window)
            return !info.popup && m_native->windowGetMovable(info.nativeId);

    return false;
}

bool Renderer::queryWindowPlacement(const Window* window, WindowSavedPlacementSetup& outPlacement) const
{
    for (auto& info : m_windows)
    {
        if (info.window == window)
        {
            if (m_native->windowGetDefaultPlacement(info.nativeId, outPlacement.rect))
            {
                outPlacement.visible = m_native->windowGetVisible(info.nativeId);
                outPlacement.minimized = m_native->windowGetMinimized(info.nativeId);
                outPlacement.maximized = m_native->windowGetMaximized(info.nativeId);
                return true;
            }
        }
    }

    return false;
}

uint64_t Renderer::queryWindowNativeHandle(const Window* window) const
{
    for (auto& info : m_windows)
        if (info.window == window)
            return m_native->windowNativeHandle(info.nativeId);

    return 0;
}

extern void PrintElementStats();

void Renderer::updateAndRender(float dt)
{
    PC_SCOPE_LVL0(UIRender);

    // process UI timers
    processTimers();

    // update current input action
    if (m_currentInputAction)
        processInputActionResult(m_currentInputAction->onUpdate(dt));

    // create native representation for windows
    {
        // create/destroy native representation for windows
        PC_SCOPE_LVL1(WindowRepresentation);
        for (auto& info : m_windows)
            updateWindowRepresentation(info);

        // remove closed windows from list
        for (int i = m_windows.lastValidIndex(); i >= 0; --i)
        {
            if (m_windows[i].nativeId == 0 && m_windows[i].closeAndRemove)
            {
                m_windowList.remove(m_windows[i].window);
                m_windows.erase(i);
            }
        }
    }

    // suck all input events
    InplaceArray<RefPtr<InputEvent>, 64> inputEvents;
    {
        PC_SCOPE_LVL1(PullInput);
        for (auto& info : m_windows)
            while (auto evt = m_native->windowPullInputEvent(info.nativeId))
                inputEvents.pushBack(evt);
    }

    // activation state
    for (auto& info : m_windows)
    {
        auto active = m_native->windowGetFocus(info.nativeId);
        if (active != info.activeState || !info.activeStateSeen)
        {
            info.activeState = active;
            info.activeStateSeen = true;
            TRACE_INFO("Active state for '{}': {}", info.window->name(), active);
            info.window->handleExternalActivation(active);
        }
    }
        
    // process inputs
    {
        PC_SCOPE_LVL1(ProcessInput);
        for (auto ptr : inputEvents)
        {
            if (const auto* evt = ptr->toMouseMoveEvent())
                processMouseMovement(*evt);
            else if (const auto* evt = ptr->toMouseClickEvent())
                processMouseClick(*evt);
            else if (const auto* evt = ptr->toMouseCaptureLostEvent())
                processMouseCaptureLostEvent(*evt);
            else if (const auto* evt = ptr->toKeyEvent())
                processKeyEvent(*evt);
            else if (const auto* evt = ptr->toCharEvent())
                processCharEvent(*evt);
            else if (const auto* evt = ptr->toAxisEvent())
                processAxisEvent(*evt);
                
        }
    }

    // update hover
    updateHoverPosition();
    updateFocusRequest();
    updateTooltip();

    // update window state
    {
        PC_SCOPE_LVL1(WindowRequests);
        for (auto& info : m_windows)
            updateWindowState(info); 
    }

    // update styles
    bool styleChanged = false;
    if (m_stash->stylesVersion() != m_stylesVersion)
    {
        m_stylesVersion = m_stash->stylesVersion();
        styleChanged = true;
    }

    // prepare windows
    {
        PC_SCOPE_LVL1(WindowLayout);
        for (auto& info : m_windows)
        {
            float pixelScale = m_native->windowPixelScale(info.nativeId);
            info.window->prepare(*m_stash, pixelScale, false, styleChanged);
        }
    }

    // render windows to canvas surfaces
    {
        PC_SCOPE_LVL1(WindowRender);
        for (auto& info : m_windows)
        {
            ElementArea windowArea;
            if (m_native->windowGetRenderableArea(info.nativeId, windowArea))
            {
                if (!info.hitCache)
                    info.hitCache.create();

                info.hitCache->reset(windowArea);

				canvas::Canvas::Setup setup;
				setup.width = std::max<uint32_t>(1, std::ceilf(windowArea.size().x));
				setup.height = std::max<uint32_t>(1, std::ceilf(windowArea.size().y));
				setup.pixelOffset.x = -windowArea.absolutePosition().x;
				setup.pixelOffset.y = -windowArea.absolutePosition().y;

				canvas::Canvas canvas(setup);
				info.window->render(canvas, *info.hitCache, *m_stash, windowArea);

				if (m_currentInputAction && m_currentInputAction->element() && m_currentInputAction->element()->findParentWindow() == info.window)
				{
					const auto pos = m_currentInputAction->element()->cachedDrawArea().absolutePosition();
					m_currentInputAction->onRender(canvas);
				}

				m_native->windowRenderContent(info.nativeId, windowArea, false, canvas);
            }
            else
            {
                info.hitCache.reset();
            }
        }
    }

    // print stats
    PrintElementStats();
}

void Renderer::resetTooltip()
{
    m_lastHoverUpdateTime = NativeTimePoint::Now();
    m_currentTooltipOwnerArea = ElementArea();
    m_currentTooltipOwner.reset();

    if (m_currentTooltip)
    {
        TRACE_SPAM("Tooltip reset");
        m_currentTooltip->requestClose();
        m_currentTooltip.reset();
    }
}

void Renderer::processTimers()
{
    PC_SCOPE_LVL0(processTimers);

    auto* cur = m_timerListHead;
    while (cur)
    {
        if (!cur->callTime.reached())
            break;

        m_timerIterationNext = cur->next;
        cur->timer->call();
        cur = m_timerIterationNext;
    }
}

static bool IsInSameHover(const ElementWeakPtr& currentHover, IElement* testElement)
{
    while (testElement)
    {
        if (currentHover == testElement)
            return true;
        testElement = testElement->parentElement();
    }

    return false;
}

static bool IsInSameHover(IElement* currentHover, IElement* testElement)
{
    while (testElement)
    {
        if (currentHover == testElement)
            return true;
        testElement = testElement->parentElement();
    }

    return false;
}

void Renderer::updateTooltip()
{
    // if we have a tooltip we close it if the owner dies
    // we also reset the tooltip if the owner is not on the hover list
    if (m_currentTooltip)
    {
        if (!m_currentTooltipOwnerArea.contains(m_lastHoverUpdatePosition))
        {
            resetTooltip();
        }
        else
        {
            auto tooltipPos = m_lastHoverUpdatePosition;
            tooltipPos.x += 10.0f;
            tooltipPos.y += 10.0f;
            m_currentTooltip->requestMove(tooltipPos);
        }
    }

    // try to create a new tooltip
    if (!m_currentTooltip && m_lastHoverUpdateTime.timeTillNow().toMiliSeconds() > cvHoverDuration.get() && !m_tooltipFrozenUntilNextMouseMove)
    {
        for (ElementChildToParentIterator it(m_currentHoverElement); it; ++it)
        {
            // allow element to handle it
            if (!it->handleHoverDuration(m_lastHoverUpdatePosition))
            {
                ElementArea tooltipArea;
                if (auto tooltip = it->queryTooltipElement(m_lastHoverUpdatePosition, tooltipArea))
                {
                    m_currentTooltipOwner = it.ptr();
                    m_currentTooltipOwnerArea = tooltipArea;

                    m_currentTooltip = RefNew<PopupWindow>();
                    m_currentTooltip->styleType("Tooltip"_id);
                    m_currentTooltip->attachChild(tooltip);
                    m_currentTooltip->show(*it, PopupWindowSetup().relativeToCursor().offset(10, 10).bottomLeft().autoClose(false).interactive(false));

                    m_currentTooltip->bind(EVENT_WINDOW_CLOSED) = [this]()
                    {
                        m_lastHoverUpdateTime = NativeTimePoint::Now();
                        m_currentTooltipOwnerArea = ElementArea();
                        m_currentTooltipOwner.reset();
                        m_currentTooltip.reset();
                    };
                }
                else
                {
                    m_lastHoverUpdateTime.resetToNow();
                }
            }
        }
    }
}

void Renderer::resetDragDrop()
{
    // destroy preview item
    if (m_currentDragDropPreviewPopup)
    {
        m_currentDragDropPreviewPopup->requestClose();
        m_currentDragDropPreviewPopup.reset();
    }

    // cancel current action
    if (m_currentDragDropHandler)
    {
        m_currentDragDropHandler->handleCancel();
        m_currentDragDropHandler.reset();
    }

    // reset other shit
    m_currentDragDropPreview.reset();
    m_currentDragDropSource.reset();
    m_currentDragDropData.reset();
    m_currentDragDropClickPos = Point();
    m_currentDragDropStarted = false;
    m_currentDragDropCanFinish = false;
}

bool Renderer::updateDragDrop(const InputMouseMovementEvent& evt)
{
    // consider if we should start a full drag
    if (!m_currentDragDropData)
        return false;

    // transform the preliminary drag&drop into a full drag&drop
    if (!m_currentDragDropStarted)
    {
        // to far
        auto dist = evt.absolutePosition().toVector().distance(m_currentDragDropClickPos.toVector());
        if (dist < 15.0f)
            return false;

        // finish the preparatory phase and create the visualization for the data
        TRACE_SPAM("Starting proper drag&drop");
        m_currentDragDropPreview = m_currentDragDropData->createPreview();
        m_currentDragDropStarted = true;

        // attach the visualization
        // TODO: track different screens
        if (m_currentDragDropPreview)
        {
            if (auto source = m_currentDragDropSource.lock())
            {
                m_currentDragDropPreviewPopup = RefNew<PopupWindow>();
                m_currentDragDropPreviewPopup->styleType("DragDropPopupWindow"_id);
                m_currentDragDropPreviewPopup->attachChild(m_currentDragDropPreview);
                m_currentDragDropPreviewPopup->show(source, PopupWindowSetup().offset(10, 10).relativeToCursor().bottomLeft().interactive(false).autoClose(false));
            }
        }
    }

    // move the preview
    if (m_currentDragDropPreview)
    {
        auto parentWindow = m_currentDragDropPreview->findParentWindow();
        auto relativePos = evt.absolutePosition().toVector();// -parentWindow->cachedDrawArea().absolutePosition();
        m_currentDragDropPreviewPopup->requestMove(relativePos + Position(10,10));
    }

    // if we do have a active handler make sure the target element is still under cursor
    if (m_currentDragDropHandler)
    {
        auto target = m_currentDragDropHandler->target();
        auto hover = m_currentHoverElement.lock();
        if (!IsInSameHover(hover, target) && !IsInSameHover(target, hover))
        {
            // reset visualization
            if (m_currentDragDropPreview)
            {
                m_currentDragDropPreview->removeStyleClass("valid"_id);
                m_currentDragDropPreview->removeStyleClass("invalid"_id);
            }

            // the element for which the drag&drop handler was created is no longer under the cursor, we have to cancel the drag for that item
            TRACE_SPAM("Current handler for drag&drop canceled because cursor exited the tracking area");
            m_currentDragDropHandler->handleCancel();
            m_currentDragDropHandler.reset();
        }
    }

    // move the preview from window to window
    /*if (m_currentDragDropPreview && !m_currentHoverElement.expired())
    {
        // get the current window that is rendering the preview overlay
        auto previewOverlayWindow = m_currentDragDropPreview->findParentWindow();

        // look at the root window
        auto rootWindow = m_hoverStack.m_elements.back().lock();
        if (rootWindow)
        {
            auto hoveredWindow = rootWindow->findParentWindow();
            if (hoveredWindow && hoveredWindow != previewOverlayWindow)
            {
                // detach from current window
                m_currentDragDropPreview->detach();

                // attach in new window
                auto relativePos = evt.absolutePosition().toVector() - rootWindow->cachedDrawArea().absolutePosition();
                m_currentDragDropPreview->move(relativePos);
                if (!AttachOverlay(rootWindow, m_currentDragDropPreview, OverlayAnchorMode::Fixed, Size(0,0), Position(10,10)))
                {
                    m_currentDragDropPreview.reset();
                }
            }
        }
    }*/

    // try to create a new handler
    if (!m_currentDragDropHandler && !m_currentHoverElement.expired())
    {
        // look in the hover stack for an element willing to service the drag data
        for (ElementChildToParentIterator it(m_currentHoverElement); it; ++it)
        {
            auto handler = it->handleDragDrop(m_currentDragDropData, evt.absolutePosition().toVector());
            if (handler)
            {
                m_currentDragDropHandler = handler;
                break;
            }
        }
    }

    // update the handler
    if (m_currentDragDropHandler)
    {
        // update the drag for the element
        auto ret = m_currentDragDropHandler->processMouseMovement(evt);
        if (ret == IDragDropHandler::UpdateResult::Cancel)
        {
            TRACE_SPAM("Current handler requested to cancel the drag action");
            m_currentDragDropHandler->handleCancel();
            m_currentDragDropHandler.reset();
            m_currentDragDropCanFinish = false;
        }
        else if (ret == IDragDropHandler::UpdateResult::Valid)
        {
            // update visualization
            if (m_currentDragDropPreviewPopup)
            {
                m_currentDragDropPreviewPopup->removeStyleClass("invalid"_id);
                m_currentDragDropPreviewPopup->addStyleClass("valid"_id);
            }
            m_currentDragDropCanFinish = true;
        }
        else if (ret == IDragDropHandler::UpdateResult::Invalid)
        {
            // update visualization
            if (m_currentDragDropPreviewPopup)
            {
                m_currentDragDropPreviewPopup->removeStyleClass("valid"_id);
                m_currentDragDropPreviewPopup->addStyleClass("invalid"_id);
            }
            m_currentDragDropCanFinish = false;
        }

        // toggle visibility
        auto canHideDefaultPreview = m_currentDragDropHandler->canHideDefaultDataPreview();
        if (m_currentDragDropPreviewPopup)
        {
            if (canHideDefaultPreview)
                m_currentDragDropPreviewPopup->requestHide();
            else
                m_currentDragDropPreviewPopup->requestShow();
        }
    }
    else
    {
        // if we don't have handler the default data preview should always be visible
        if (m_currentDragDropPreviewPopup)
        {
            m_currentDragDropPreviewPopup->removeStyleClass("invalid"_id);
            m_currentDragDropPreviewPopup->removeStyleClass("valid"_id);
            m_currentDragDropPreviewPopup->requestShow();
        }
    }

    // mouse wheel
    if (m_currentDragDropHandler && evt.delta().z != 0.0f)
    {
        auto mouseWheel = evt.delta().z;
        m_currentDragDropHandler->handleMouseWheel(evt, mouseWheel);
    }

    // eat the even
    return true;
}

void Renderer::processMouseMovement(const InputMouseMovementEvent& evt)
{
    // track position
    m_lastMouseMovementPosition = evt.absolutePosition().toVector();

    // update the drag&drop shit
    if (updateDragDrop(evt))
        return;

    // if input action is active it consumes all mouse events
    if (m_currentInputAction)
        if (processInputActionResult(m_currentInputAction->onMouseMovement(evt, m_currentHoverElement)))
            return;

    // send the mouse movement, mouse wheel is sent differently (focus free)
    auto mouseWheel = evt.delta().z;
    if (mouseWheel != 0.0f)
    {
        for (ElementChildToParentIterator it(m_currentHoverElement); it; ++it)
            if (it->handleMouseWheel(evt, mouseWheel))
                break;
    }
    else
    {
        for (ElementChildToParentIterator it(m_currentHoverElement); it; ++it)
            if (it->handleMouseMovement(evt))
                break;
    }
}

void Renderer::processMouseClick(const InputMouseClickEvent& evt)
{
    // always track the focus element
    //updateHoverPosition(evt.absolutePosition().toVector());

    // cancel any drag&drop actions
    if (evt.leftReleased())
    {
        if (m_currentDragDropCanFinish && m_currentDragDropHandler)
            m_currentDragDropHandler->handleData(evt.absolutePosition().toVector());
        resetDragDrop();
    }

    // track context menu
    if (evt.rightClicked())
    {
        m_potentialContextMenuElementClickPos = m_lastHoverUpdatePosition;
        m_potentialContextMenuElementPtr = m_currentHoverElement;
        m_lastMouseMovementPosition = evt.absolutePosition().toVector();
    }

    // if input action is active it consumes all mouse events
    if (m_currentInputAction)
        if (processInputActionResult(m_currentInputAction->onMouseEvent(evt, m_currentHoverElement)))
            return;

    // trace the full stack
    const auto absolutePosition = evt.absolutePosition().toVector();
    if (auto wnd = windowAtPos(absolutePosition))
    {
        if (wnd->hitCache)
        {
            auto elementStack = wnd->hitCache->traceElement(absolutePosition);

            // process the stack in the reverse order to handle the overlay mouse events
            bool overlayClickChangedFocus = false;
            if (!m_currentInputAction)
            {
                uint32_t lastFocusCounter = m_currentFocusCounter;
                for (ElementParentToChildIterator it(elementStack); it; ++it)
                {
                    auto inputHandler = it->handleOverlayMouseClick(it->cachedDrawArea(), evt);
                    if (inputHandler)
                    {
                        m_currentInputAction = inputHandler;
                        break;
                    }
                }

                if (m_currentFocusCounter != lastFocusCounter)
                    overlayClickChangedFocus = true;
            }

            // get the mouse focus element unless the focus was already handled by the overlay click (like in lists/trees)
            if (!m_currentInputAction && !overlayClickChangedFocus)
            {
                if (evt.leftClicked() || evt.rightClicked())
                {
                    IElement* newFocus = nullptr;
                    for (ElementChildToParentIterator it(elementStack); it; ++it)
                    {
                        if (it->isAllowingFocusFromClick())
                        {
                            newFocus = *it;
                            break;
                        }
                    }

                    focus(newFocus);
                }
            }

            // process from the end and walk the stack up until event is handled by something
            if (!m_currentInputAction)
            {
                for (ElementChildToParentIterator it(elementStack); it; ++it)
                {
                    if (auto ret = it->handleMouseClick(it->cachedDrawArea(), evt))
                    {
                        processInputActionResult(ret);
                        break;
                    }
                }
            }

            // if no input action was created start preparations for the drag&drop operation
            if (!m_currentInputAction && !elementStack.empty() && evt.leftClicked())
            {
                for (ElementChildToParentIterator it(elementStack); it; ++it)
                {
                    auto dragData = it->queryDragDropData(evt.keyMask(), absolutePosition);
                    if (dragData)
                    {
                        m_currentDragDropClickPos = evt.absolutePosition();
                        m_currentDragDropSource = it.ptr();
                        m_currentDragDropData = dragData;
                        break;
                    }
                }
            }
        }
    }

    // display context menu
    // the test is done here to allow the input action to finish first
    if (evt.rightReleased() && !m_currentInputAction)
    {
        auto target = m_potentialContextMenuElementPtr.lock();
        m_potentialContextMenuElementPtr.reset();

        // if we are close to the initial click try to display a context menu
        if (target)
        {
            auto currentPos = m_lastMouseMovementPosition;
            if (currentPos.distance(m_potentialContextMenuElementClickPos.toVector()) < 5.0f)
            {
                target->handleContextMenu(target->cachedDrawArea(), currentPos, evt.keyMask().mask());
            }
        }
    }
}

void Renderer::processMouseCaptureLostEvent(const InputMouseCaptureLostEvent& evt)
{
    if (m_currentInputAction)
    {
        TRACE_INFO("Canceling mouse action since capture was lost");
        m_currentInputAction->onCanceled();
        m_currentInputAction.reset();
    }
}

void Renderer::processKeyEvent(const InputKeyEvent& evt)
{
    // pressing any key revokes our chance to get the context menu
    if (evt.pressed() && evt.keyCode() != InputKey::KEY_MOUSE1)
        m_potentialContextMenuElementPtr.reset();

    // pressing any key cancels the drag action
    if (m_currentDragDropHandler)
    {
        if (evt.pressed() && evt.keyCode() == InputKey::KEY_ESCAPE)
        {
            resetDragDrop();
            return;
        }
    }

    // pass to input action if it exists
    if (m_currentInputAction)
        if (processInputActionResult(m_currentInputAction->onKeyEvent(evt)))
            return;

    //TRACE_INFO("InputKeyEvent: {} {} {}", evt.pressed(), evt.released(), evt.keyCode());

    // send the preview event to the controls in the reverse order
    // NOTE: the preview is not sent to the focused element
    // handle preview - back to front
    bool handled = false;
    for (ElementParentToChildIterator it(m_currentFocusElement); it; ++it)
    {
        //TRACE_INFO("Preview at {} ({})", *it, it->cls()->name());
        if (m_currentFocusElement != *it && it->previewKeyEvent(evt))
        {
            //TRACE_INFO("PreviewHandled at {} ({})", *it, it->cls()->name());
            handled = true;
            break;
        }
    }

    // send the events in normal order
    if (!handled)
    {
        for (ElementChildToParentIterator it(m_currentFocusElement); it; ++it)
        {
            //TRACE_INFO("Process at {} ({})", *it, it->cls()->name());
            if (it->handleKeyEvent(evt))
            {
                //TRACE_INFO("Handled at {} ({})", *it, it->cls()->name());
                handled = true;
                break;
            }
        }
    }

    // TODO: element navigation
    if (!handled)
    {

    }
}

void Renderer::processCharEvent(const InputCharEvent& evt)
{
    // pressing any key cancels the drag action
    resetDragDrop();

    // during active input action the chars are ignored
    if (m_currentInputAction)
        return;

    // send the char event to the focused control stack
    for (ElementChildToParentIterator it(m_currentFocusElement); it; ++it)
        if (it->handleCharEvent(evt))
            break;
}

void Renderer::processAxisEvent(const InputAxisEvent& evt)
{
    // during active input action the chars are ignored
    if (m_currentInputAction)
    {
        m_currentInputAction->onAxisEvent(evt);
        return;
    }

    // send the char event to the focused control stack
    for (ElementChildToParentIterator it(m_currentFocusElement); it; ++it)
        if (it->handleAxisEvent(evt))
            break;
}

bool Renderer::processInputActionResult(const InputActionResult& result)
{
    resetTooltip();

    auto prevCaptureWindowId = m_currentInputAction ? nativeWindowForElement(m_currentInputAction->element().get()) : 0;
    auto prevCaptureMode = m_currentInputAction ? (m_currentInputAction->fullMouseCapture() ? 2 : 1) : 0;

    if (result.resultCode() == InputActionResult::ResultCode::Exit)
        m_currentInputAction.reset();
    else if (result.resultCode() == InputActionResult::ResultCode::Replace)
        m_currentInputAction = (result.nextAction() != IInputAction::CONSUME()) ? result.nextAction() : nullptr;

    auto captureWindowId = m_currentInputAction ? nativeWindowForElement(m_currentInputAction->element().get()) : 0;
    auto captureMode = m_currentInputAction ? (m_currentInputAction->fullMouseCapture() ? 2 : 1) : 0;

    if (captureWindowId != prevCaptureWindowId || captureMode != prevCaptureMode)
    {
        TRACE_INFO("Changing UI capture mode to {} for window {}", captureMode, captureWindowId);

        if (captureWindowId != prevCaptureWindowId && prevCaptureMode != 0)
            m_native->windowSetCapture(prevCaptureWindowId, 0);

        if (captureWindowId != 0 && captureMode != 0)
            m_native->windowSetCapture(captureWindowId, captureMode);
    }

    return !!m_currentInputAction;
}

//--

bool Renderer::nativeWindowSelectCursor(NativeWindowID id, const Position& absolutePosition, CursorType& outCursorType)
{
    for (auto& window : m_windows)
    {
        if (window.nativeId == id)
        {
            if (m_currentInputAction)
            {
                auto owner = m_currentInputAction->element();
                if (!owner || owner->findParentWindow() == window.window)
                {
                    m_currentInputAction->onUpdateCursor(outCursorType);
                    return true;
                }
            }

            if (window.hitCache)
            {
                if (const auto element = window.hitCache->traceElement(absolutePosition))
                {
                    for (ElementChildToParentIterator it(element.get()); it; ++it)
                    {
                        if (element->handleCursorQuery(element->cachedDrawArea(), absolutePosition, outCursorType))
                            return true;
                    }
                }
            }

            break;
        }
    }

    return false;
}

bool Renderer::nativeWindowHitTestNonClientArea(NativeWindowID id, const Position& absolutePosition, AreaType& outAreaType)
{
    for (auto& window : m_windows)
    {
        if (window.nativeId == id)
        {
            // allow window's frame to always be tested first - helps with shit that is very close to the frame's border
            if (window.window->handleWindowFrameArea(window.window->cachedDrawArea(), absolutePosition, outAreaType))
                return true;

            // if window frame did not hijack the request pass it to normal members
            if (window.hitCache)
                if (const auto element = window.hitCache->traceElement(absolutePosition))
                    for (ElementChildToParentIterator it(element.get()); it; ++it)
                        if (element->handleWindowAreaQuery(element->cachedDrawArea(), absolutePosition, outAreaType))
                            return true;

            break;
        }
    }

    return false;
}

void Renderer::nativeWindowForceRedraw(NativeWindowID id)
{
	PC_SCOPE_LVL1(RenderForceRedraw);

    for (auto& info : m_windows)
    {
        if (info.nativeId == id)
        {
            ElementArea windowArea;
            if (m_native->windowGetRenderableArea(info.nativeId, windowArea))
            {
                if (!info.hitCache)
                    info.hitCache.create();

                info.hitCache->reset(windowArea);

                {
                    const auto pixelScale = m_native->windowPixelScale(info.nativeId);
                    info.window->prepare(*m_stash, pixelScale, false, false);
                }

                //TRACE_INFO("ForcedRedrawSize: [{}x{}]", windowArea.size().x, windowArea.size().y);
                //Sleep(200);

				canvas::Canvas::Setup setup;
				setup.width = std::max<uint32_t>(1, std::ceilf(windowArea.size().x));
				setup.height = std::max<uint32_t>(1, std::ceilf(windowArea.size().y));
				setup.pixelOffset.x = -windowArea.absolutePosition().x;
				setup.pixelOffset.y = -windowArea.absolutePosition().y;

				canvas::Canvas canvas(setup);
				info.window->render(canvas, *info.hitCache, *m_stash, windowArea);

				if (m_currentInputAction && m_currentInputAction->element() && m_currentInputAction->element()->findParentWindow() == info.window)
				{
					const auto pos = m_currentInputAction->element()->cachedDrawArea().absolutePosition();
					m_currentInputAction->onRender(canvas);
				}

				m_native->windowRenderContent(info.nativeId, windowArea, true, canvas);
            }
        }
    }
}

//--

void Renderer::storeText(StringView text)
{
    UTF16StringVector wideString(text);
    m_native->stroreClipboardData("UNITEXT", wideString.c_str(), (1+wideString.length()) * sizeof(wchar_t));
}

bool Renderer::loadText(StringBuf& outText) const
{
    // try unicode text first, most common
    if (auto uniTextData = m_native->loadClipboardData("UNITEXT"))
    {
        auto textView = BaseStringView<wchar_t>((const wchar_t*)uniTextData.data());
        outText = StringBuf(textView);
        return true;
    }

    // try ANSI text first, some cases
    if (auto ansiTextData = m_native->loadClipboardData("TEXT"))
    {
        auto textView = StringView((const char*)ansiTextData.data());
        outText = StringBuf(textView);
        return true;
    }

    // no text
    return false;
}

void Renderer::storeData(StringView format, const void* data, uint32_t size)
{
    m_native->stroreClipboardData(format, data, size);
}

bool Renderer::loadData(StringView format, Buffer& outData) const
{
    if (auto binaryData = m_native->loadClipboardData(format))
    {
        outData = binaryData;
        return true;
    }

    return false;
}

//--

void Renderer::playSound(MessageType type)
{
    m_native->playSound(type);
}

//--

END_BOOMER_NAMESPACE_EX(ui)
