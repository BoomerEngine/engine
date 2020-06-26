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

#include "base/canvas/include/canvas.h"
#include "uiRenderer.h"
#include "uiTextLabel.h"
#include "uiButton.h"

namespace ui
{
    //--

    RTTI_BEGIN_TYPE_CLASS(Window);
    RTTI_METADATA(ElementClassNameMetadata).name("Window");
    RTTI_END_TYPE();

    Window::Window()
        : m_title("Boomer Engine")
    {
        hitTest(HitTestState::Enabled);
        layoutMode(LayoutMode::Vertical);
        enableAutoExpand(true, true);
        allowFocusFromClick(false);
        allowFocusFromKeyboard(false);
        allowHover(false);
    }

    Window::~Window()
    {
        MemDelete(m_requests);
        m_requests = nullptr;

        TRACE_INFO("Window destroyed");
    }

    void Window::ForceRedrawOfEverything()
    {
    }


    WindowRequests* Window::createPendingRequests()
    {
        if (!m_requests)
            m_requests = MemNew(WindowRequests);
        return m_requests;
    }

    WindowRequests* Window::consumePendingRequests()
    {
        auto ret = m_requests;
        m_requests = nullptr;
        return ret;
    }

    void Window::requestClose()
    {
        auto requests = createPendingRequests();
        if (!requests->requestClose)
        {
            TRACE_INFO("Requested to close window '{}'", this->name());
            requests->requestClose = true;
            call("OnClosed"_id);

            if (auto owner = m_modalOwner.lock())
                owner->focus();
        }
    }

    void Window::requestActivate()
    {
        auto requests = createPendingRequests();
        if (!requests->requestActivation)
        {
            TRACE_INFO("Requested to activate window '{}'", this->name());
            requests->requestActivation = true;
        }
    }

    void Window::requestMaximize(bool restore)
    {
        auto requests = createPendingRequests();
        if (!requests->requestMaximize)
        {
            TRACE_INFO("Requested to maximize window '{}'", this->name());
            requests->requestMaximize = true;
        }
    }

    void Window::requestMinimize()
    {
        auto requests = createPendingRequests();
        if (!requests->requestMinimize)
        {
            TRACE_INFO("Requested to minimize window '{}'", this->name());
            requests->requestMinimize = true;
        }
    }

    void Window::requestMove(const Position& screenPosition)
    {
        auto requests = createPendingRequests();
        if (!requests->requestPositionChange)
        {
            //TRACE_INFO("Requested to move window '{}' to [{},{}]", this->name(), screenPosition.x, screenPosition.y);
            requests->requestPositionChange = true;
        }

        requests->requestedPosition = screenPosition;
    }

    void Window::requestSize(const Size& screenSize)
    {
        auto requests = createPendingRequests();
        if (!requests->requestSizeChange)
        {
            //TRACE_INFO("Requested to resize window '{}' to [{},{}]", this->name(), screenSize.x, screenSize.y);
            requests->requestSizeChange = true;
        }

        requests->requestedSize = screenSize;
    }

    void Window::requestTitleChange(base::StringView<char> newTitle)
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

    void Window::render(base::canvas::Canvas& canvas, HitCache& hitCache, const ElementArea& nativeArea)
    {
        PC_SCOPE_LVL1(RenderWindow);

        // prepare clip
        canvas.scissorRect(nativeArea.absolutePosition(), nativeArea.size());

        // render into provided area
        IElement::render(hitCache, nativeArea, nativeArea, canvas, 1.0f, nullptr);
    }

    void Window::handleExternalCloseRequest()
    {
        requestClose();
    }

    void Window::renderBackground(const ElementArea& drawArea, base::canvas::Canvas& canvas, float mergedOpacity)
    {
        TBaseClass::renderBackground(drawArea, canvas, mergedOpacity);

        //if (mergedOpacity)

        if (m_clearColorHack)
            canvas.clearColor(m_clearColor);
    }

    void Window::prepareBackgroundGeometry(const ElementArea& drawArea, float pixelScale, base::canvas::GeometryBuilder& builder) const
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
                TBaseClass::prepareBackgroundGeometry(drawArea, pixelScale, builder);
            }
        }
    }

    Window* Window::findParentWindow() const
    {
        return parentElement() ? parentElement()->findParentWindow() : const_cast<Window*>(this);
    }

    void Window::handleExternalActivation(bool isActive)
    {
        /*if (isActive)
            addStyleClass("active"_id);
        else
            removeStyleClass("active"_id);*/
    }

    void Window::handleStateUpdate(WindowStateFlags flags)
    {
        if (auto titleBar = findChildByName<WindowTitleBar>(""_id))
        {
            auto titleBarFlags = flags;
            if (!m_hasCloseButton)
                titleBarFlags -= WindowStateFlagBit::CanClose;
            if (!m_hasMaximizeButton)
                titleBarFlags -= WindowStateFlagBit::CanMaximize;
            if (!m_hasMinimizeButton)
                titleBarFlags -= WindowStateFlagBit::CanMinimize;
            titleBar->updateWindowState(this, flags);
        }
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

    bool Window::handleTemplateProperty(base::StringView<char> name, base::StringView<char> value)
    {
        if (name == "title")
        {
            m_title = base::StringBuf(value);
            return true;
        }
        else if (name == "resizable")
        {
            return base::MatchResult::OK == value.match(m_resizable);
        }
        else if (name == "canMaximize")
        {
            return base::MatchResult::OK == value.match(m_hasMaximizeButton);
        }
        else if (name == "canMinimize")
        {
            return base::MatchResult::OK == value.match(m_hasMinimizeButton);
        }
        else if (name == "canClose")
        {
            return base::MatchResult::OK == value.match(m_hasCloseButton);
        }

        return TBaseClass::handleTemplateProperty(name, value);
    }

    /*void Window::restoreConfiguration(WindowInitialPlacementSetup& outSetup)
    {
        auto configKey = configKey();
        if (configKey.empty())
            return;

        auto fullConfigKey = base::StringBuf("Windows/") + configKey;

        // load the window size
        if (Config::GetTypedValue<float>(fullConfigKey.c_str(), "Width", outSetup.m_initialSize.x) &&
            Config::GetTypedValue<float>(fullConfigKey.c_str(), "Height", outSetup.m_initialSize.y))
        {
            outSetup.m_sizingMode = WindowSizing::FreeSize; // use the specified window size
        }

        // load the window position
        Position pos;
        if (Config::GetTypedValue<float>(fullConfigKey.c_str(), "X", pos.x) && Config::GetTypedValue<float>(fullConfigKey.c_str(), "Y", pos.y))
        {
            m_absolutePosition = pos;
        }

        // is the window maximized ?
        if (Config::GetTypedValue<bool>(fullConfigKey.c_str(), "Maximized", false))
        {
            outSetup.m_placementMode = WindowPlacement::WholeCurrentArea;
        }
    }*/

    void Window::queryCurrentPlacementForSaving(WindowSavedPlacementSetup& outPlacement) const
    {
    }

    void Window::queryInitialPlacementSetup(WindowInitialPlacementSetup& outSetup) const
    {
        outSetup.title = m_title;
        outSetup.flagAllowResize = m_resizable;
        outSetup.flagForceActive = true;
        outSetup.flagShowOnTaskBar = true;
        outSetup.flagPopup = false;
        outSetup.flagTopMost = false;
    }

    bool Window::queryMovableState() const
    {
        if (auto* render = renderer())
            return render->queryWindowMovableState(const_cast<Window*>(this));

        return false;
    }

    bool Window::queryResizableState() const
    {
        if (auto* render = renderer())
            return render->queryWindowResizableState(const_cast<Window*>(this));

        return false;
    }

    /*base::StringBuf Window::configKey() const
    {
        return base::StringBuf::EMPTY();
    }

    void Window::storeConfiguration()
    {
        auto configKey = configKey();
        if (configKey.empty())
            return;

        auto fullConfigKey = base::StringBuf("Windows/") + configKey;

        Config::SetTypedValue<float>(fullConfigKey.c_str(), "Width", m_absoluteSize.x);
        Config::SetTypedValue<float>(fullConfigKey.c_str(), "Height", m_absoluteSize.y);
        Config::SetTypedValue<float>(fullConfigKey.c_str(), "X", m_absolutePosition.x);
        Config::SetTypedValue<float>(fullConfigKey.c_str(), "Y", m_absolutePosition.y);

        bool isMaximized = m_nativeWindow && m_nativeWindow->isMaximized();
        Config::SetTypedValue<bool>(fullConfigKey.c_str(), "Maximized", isMaximized);

        bool isMinimized = m_nativeWindow && m_nativeWindow->isMinimized();
        Config::SetTypedValue<bool>(fullConfigKey.c_str(), "Minimized", isMinimized);
    }*/

    /*void Window::attachOverlay(const base::RefPtr<WindowOverlay>& overlay)
    {
        ASSERT_EX(overlay, "Invalid overlay window");
        ASSERT_EX(!m_overlays.contains(overlay), "Overlay window already added");
        m_overlays.pushBack(overlay);
        attachChild(overlay);
    }

    void Window::dettachOverlay(const base::RefPtr<WindowOverlay>& overlay)
    {
        ASSERT_EX(m_overlays.contains(overlay), "Overlay window not added");
        m_overlays.remove(overlay);
        detachChild(overlay);
    }*/

    /*void Window::arrangeChildren(const ElementArea& innerArea, const ElementArea& clipArea, ArrangedChildren& outArrangedChildren, const ElementDynamicSizing* dynamicSizing) const
    {
        TBaseClass::arrangeChildren(innerArea, clipArea, outArrangedChildren, dynamicSizing);

        if (nullptr != m_localNativeWindow)
        {
            for (const auto& overlay : m_overlays)
            {
                if (overlay->visibility() == VisibilityState::Visible)
                {
                    auto windowArea = m_localNativeWindow->queryClientArea();
                    auto area = ElementArea(windowArea.absolutePosition() + overlay->overlayPosition(), overlay->overlaySize());
                    outArrangedChildren.add(overlay, area, area);
                }
            }
        }
    }*/

    //--

    /*void Window::manageMessages()
    {
        PC_SCOPE_LVL1(ManageMessages);

        // remove dead messages
        for (int i=m_overlays.lastValidIndex(); i >= 0; --i)
        {
			const auto& overlay = m_overlays[i];
            if (overlay && overlay->hasFinishedAnimating())
            {
                detachChild(overlay);
                m_overlays.erase(i);
            }
        }

        // sort by priority
        //std::sort(m_messages.begin(), m_messages.end(), [](const WindowMessagePtr& a, const WindowMessagePtr& b) { return a->priority() < b->priority(); });

        // place the messages in the window
        float rightEdge = cachedDrawArea().size().x - 10.0f;
        float bottomEdge = cachedDrawArea().size().y - 10.0f;
        for (int i=m_overlays.lastValidIndex(); i >= 0; --i)
        {
            auto msg = base::rtti_cast<WindowMessage>(m_overlays[i]);
            if (msg)
            {
                auto windowLeft = rightEdge - msg->overlaySize().x;
                auto windowTop = bottomEdge - msg->overlaySize().y;
                msg->move(ui::Position(windowLeft, windowTop));
                bottomEdge = windowTop;
            }
        }
    }*/

    void Window::showModal(IElement* owner)
    {
        if (m_requests && m_requests->requestClose)
            return;

        DEBUG_CHECK_EX(!renderer(), "Cannot show popup that is already visible");
        if (renderer())
            return;

        DEBUG_CHECK_EX(owner, "Owner is required for popups");
        if (!owner)
            return;

        auto renderer = owner->renderer();
        DEBUG_CHECK_EX(renderer, "Cannot create popups for elements that are not attached to renderig hierarchy");
        if (renderer)
            renderer->attachWindow(this);

        m_modalOwner = owner;
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(PopupWindow);
        RTTI_METADATA(ElementClassNameMetadata).name("PopupWindow");
    RTTI_END_TYPE();

    PopupWindow::PopupWindow()
    {
        m_resizable = false;
    }

    void PopupWindow::show(IElement* owner, const PopupWindowSetup& setup /*= PopupWindowSetup()*/)
    {
        if (m_requests && m_requests->requestClose)
            return;

        DEBUG_CHECK_EX(!renderer(), "Cannot show popup that is already visible");
        if (renderer())
            return;

        DEBUG_CHECK_EX(owner, "Owner is required for popups");
        if (!owner)
            return;

        m_parent = owner;
        m_setup = setup;

        if (m_setup.m_popupOwner.empty())
           m_setup.m_popupOwner = owner->findParentWindow();

        auto renderer = owner->renderer();
        DEBUG_CHECK_EX(renderer, "Cannot create popups for elements that are not attached to renderig hierarchy");
        if (renderer)
            renderer->attachWindow(this);
    }

    void PopupWindow::handleExternalActivation(bool isActive)
    {
        TBaseClass::handleExternalActivation(isActive);

        if (!isActive && m_setup.m_closeWhenDeactivated)
            requestClose();
    }

    bool PopupWindow::runAction(base::StringID name)
    {
        if (TBaseClass::runAction(name))
            return true;

        if (auto parent = m_parent.lock())
            return parent->runAction(name);

        if (auto owner = m_setup.m_popupOwner.lock())
            return owner->runAction(name);

        return false;
    }

    ActionStatusFlags PopupWindow::checkAction(base::StringID name) const
    {
        ActionStatusFlags ret = TBaseClass::checkAction(name);

        if (auto parent = m_parent.lock())
            ret |= parent->checkAction(name);

        if (auto owner = m_setup.m_popupOwner.lock())
            ret |= owner->checkAction(name);

        return ret;
    }

    bool PopupWindow::queryResizableState() const
    {
        return false;
    }

    void PopupWindow::queryInitialPlacementSetup(WindowInitialPlacementSetup& outSetup) const
    {
        outSetup.flagPopup = true;
        outSetup.flagShowOnTaskBar = false;
        outSetup.flagTopMost = false;
        outSetup.flagForceActive = m_setup.m_closeWhenDeactivated; // if we want to auto close the popup once focus is lost we need to make sure window is auto-activated
        outSetup.flagRelativeToCursor = m_setup.m_relativeToCursor;
        outSetup.mode = m_setup.m_mode;
        outSetup.referenceElement = m_parent;
        outSetup.owner = m_setup.m_popupOwner;
        outSetup.offset = m_setup.m_initialOffset;
        outSetup.size = m_setup.m_initialSize;
    }

    //--

    RTTI_BEGIN_TYPE_CLASS(WindowTitleBar);
        RTTI_METADATA(ElementClassNameMetadata).name("WindowTitleBar");
    RTTI_END_TYPE();

    WindowTitleBar::WindowTitleBar(base::StringView<char> title)
    {
        hitTest(HitTestState::Enabled);
        layoutMode(LayoutMode::Horizontal);

        {
            auto appIcon = createNamedChild<Image>("AppIcon"_id);
            appIcon->hitTest(true);
            appIcon->customStyle("windowAreaType"_id, base::input::AreaType::SysMenu);
        }

        {
            m_title = createNamedChild<TextLabel>("Caption"_id, title);
        }

        {
            auto buttonCluster = createNamedChild<>("ButtonContainer"_id);
            buttonCluster->layoutHorizontal();

            m_minimizeButton = buttonCluster->createNamedChild<Button>("Minimize"_id);
            m_minimizeButton->attachChild(base::CreateSharedPtr<TextLabel>());
            m_minimizeButton->visibility(false);

            m_maximizeButton = buttonCluster->createNamedChild<Button>("Maximize"_id);
            m_maximizeButton->attachChild(base::CreateSharedPtr<TextLabel>());
            m_maximizeButton->visibility(false);

            m_closeButton = buttonCluster->createNamedChild<Button>("Close"_id);
            m_closeButton->attachChild(base::CreateSharedPtr<TextLabel>());

            //m_minimizeButton->customStyle("windowAreaType"_id, base::input::AreaType::Minimize);
            //m_maximizeButton->customStyle("windowAreaType"_id, base::input::AreaType::Maximize);
            //m_closeButton->customStyle("windowAreaType"_id, base::input::AreaType::Close);

            m_closeButton->bind("OnClick"_id, this) = [](WindowTitleBar* bar)
            {
                if (auto* window = bar->findParentWindow())
                    window->handleExternalCloseRequest();
            };

            m_maximizeButton->bind("OnClick"_id, this) = [](WindowTitleBar* bar)
            {
                if (auto window = bar->findParentWindow())
                    window->requestMaximize(true);
            };

            m_minimizeButton->bind("OnClick"_id, this) = [](WindowTitleBar* bar)
            {
                if (auto window = bar->findParentWindow())
                    window->requestMinimize();
            };
        }
    }

    WindowTitleBar::~WindowTitleBar()
    {}

    void WindowTitleBar::updateWindowState(ui::Window* window, WindowStateFlags flags)
    {
        if (m_maximizeButton)
            m_maximizeButton->visibility(flags.test(WindowStateFlagBit::CanMaximize));

        if (m_minimizeButton)
            m_minimizeButton->visibility(flags.test(WindowStateFlagBit::CanMinimize));

        if (m_closeButton)
            m_closeButton->visibility(flags.test(WindowStateFlagBit::CanClose));

        if (m_title)
            m_title->text(window->title());
    }

    bool WindowTitleBar::handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const
    {
        outAreaType = base::input::AreaType::Caption;
        return true;
    }

    //--

} // ui