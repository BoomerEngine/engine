/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\window #]
***/

#include "build.h"
#include "uiWindowPopup.h"
#include "uiRenderer.h"

namespace ui
{

    //--

    RTTI_BEGIN_TYPE_CLASS(PopupWindow);
        RTTI_METADATA(ElementClassNameMetadata).name("PopupWindow");
    RTTI_END_TYPE();

    PopupWindow::PopupWindow(WindowFeatureFlags flags, base::StringView<char> title)
        : Window(flags, title)
    {        
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

    bool PopupWindow::runAction(base::StringID name, IElement* source)
    {
        if (TBaseClass::runAction(name, source))
            return true;

        if (auto parent = m_parent.lock())
            return parent->runAction(name, source);

        if (auto owner = m_setup.m_popupOwner.lock())
            return owner->runAction(name, source);

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

} // ui