/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\toolbar #]
***/

#pragma once

#include "uiElement.h"

namespace ui
{
    //--

    /// toolbar button
    struct ToolbarButtonSetup
    {
        base::StringView m_icon;
        base::StringView m_caption;
        base::StringView m_tooltip;

        INLINE bool valid() const { return m_icon || m_caption; }

        INLINE ToolbarButtonSetup() {}
        INLINE ToolbarButtonSetup& icon(base::StringView txt) { m_icon = txt; return *this; };
        INLINE ToolbarButtonSetup& caption(base::StringView txt) { m_caption = txt; return *this; };
        INLINE ToolbarButtonSetup& tooltip(base::StringView txt) { m_tooltip = txt; return *this; };
    };

    //--

    /// toolbar - collection of tool elements
    class BASE_UI_API ToolBar : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(ToolBar, IElement);

    public:
        ToolBar();

        // add a vertical separator to the toolbar
        void createSeparator();

        // add a simple tool button
        void createButton(base::StringID action, const ToolbarButtonSetup& setup);

        // add a simple tool button with direct action
        EventFunctionBinder createCallback(const ToolbarButtonSetup& setup);

        // update caption on a button
        void updateButtonCaption(base::StringID action, const ToolbarButtonSetup& setup);

    protected:
        Timer m_timerUpdateState;

        base::HashMap<base::StringID, ButtonPtr> m_actionButtons;

        void updateButtonState();

        virtual void attachChild(IElement* childElement) override;
        virtual void detachChild(IElement* childElement) override;

        virtual bool handleTemplateProperty(base::StringView name, base::StringView value) override;
        virtual bool handleTemplateChild(base::StringView name, const base::xml::IDocument& doc, const base::xml::NodeID& id) override;
    };

    //--

} // ui