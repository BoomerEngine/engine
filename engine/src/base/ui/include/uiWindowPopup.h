/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\window #]
***/

#pragma once

#include "uiWindow.h"

namespace ui
{
    
    //--

    /// popup window setup
    struct PopupWindowSetup
    {
        WindowInitialPlacementMode m_mode = WindowInitialPlacementMode::BottomLeft;
        Size m_initialSize = Size(0, 0); // use content size 
        Position m_initialOffset = Position(0, 0); // no offset
        bool m_interactive = true;
        bool m_closeWhenDeactivated = true;
        bool m_relativeToCursor = false;
        WindowWeakPtr m_popupOwner;

        INLINE PopupWindowSetup() {};
        INLINE PopupWindowSetup& relativeToCursor() { m_relativeToCursor = true; return *this; }
        INLINE PopupWindowSetup& size(Size size) { m_initialSize = size; return *this; }
        INLINE PopupWindowSetup& size(float x, float y) { m_initialSize = Size(x, y); return *this; }
        INLINE PopupWindowSetup& offset(Position pos) { m_initialOffset = pos; return *this; }
        INLINE PopupWindowSetup& offset(float x, float y) { m_initialOffset = Position(x, y); return *this; }
        INLINE PopupWindowSetup& mode(WindowInitialPlacementMode mode) { m_mode = mode; return *this; }
        INLINE PopupWindowSetup& bottomLeft() { m_mode = WindowInitialPlacementMode::BottomLeft; return *this; }
        INLINE PopupWindowSetup& bottomRight() { m_mode = WindowInitialPlacementMode::BottomRight; return *this; }
        INLINE PopupWindowSetup& topLeft() { m_mode = WindowInitialPlacementMode::TopLeft; return *this; }
        INLINE PopupWindowSetup& topRight() { m_mode = WindowInitialPlacementMode::TopRight; return *this; }
        INLINE PopupWindowSetup& topRightNeighbour() { m_mode = WindowInitialPlacementMode::TopRightNeighbour; return *this; }
        INLINE PopupWindowSetup& center() { m_mode = WindowInitialPlacementMode::WindowCenter; return *this; }
        INLINE PopupWindowSetup& screenCenter() { m_mode = WindowInitialPlacementMode::ScreenCenter; return *this; }
        INLINE PopupWindowSetup& areaCenter() { m_mode = WindowInitialPlacementMode::AreaCenter; return *this; }
        INLINE PopupWindowSetup& interactive(bool flag=true) { m_interactive = flag; return *this; }
        INLINE PopupWindowSetup& autoClose(bool flag=true) { m_closeWhenDeactivated = flag; return *this; }
        INLINE PopupWindowSetup& owner(Window* window) { m_popupOwner = window; }
    };

    /// popup is a very specific simple window that will close when deactivated, does not have a title bar, or show anywhere
    class BASE_UI_API PopupWindow : public Window
    {
        RTTI_DECLARE_VIRTUAL_CLASS(PopupWindow, Window);

    public:
        PopupWindow(WindowFeatureFlags flags = WindowFeatureFlagBit::DEFAULT_POPUP, base::StringView<char> title="");

        /// get owner of this popup, this is needed only so we restore the focus to the right place
        INLINE const WindowWeakPtr& owner() const { return m_setup.m_popupOwner; }

        /// setup popup position/size to pop at, shows the popup
        void show(IElement* owner, const PopupWindowSetup& setup = PopupWindowSetup());

    protected:
        ElementWeakPtr m_parent; // we auto close if parent is gone
        PopupWindowSetup m_setup;

        virtual bool queryResizableState() const override;
        virtual void queryInitialPlacementSetup(WindowInitialPlacementSetup& outSetup) const override;
        virtual void handleExternalActivation(bool isActive) override;
        virtual bool runAction(base::StringID name, IElement* source) override;
        virtual ActionStatusFlags checkAction(base::StringID name) const override;
    };

    //--

} // ui