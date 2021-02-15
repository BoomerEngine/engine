/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\window #]
***/

#include "build.h"
#include "uiWindow.h"
#include "uiWindowTitlebar.h"
#include "uiImage.h"
#include "uiTextLabel.h"
#include "uiButton.h"

namespace ui
{
    //--

    RTTI_BEGIN_TYPE_NATIVE_CLASS(WindowTitleBar);
        RTTI_METADATA(ElementClassNameMetadata).name("WindowTitleBar");
    RTTI_END_TYPE();

    WindowTitleBar::WindowTitleBar(WindowFeatureFlags flags, base::StringView title)
    {
        hitTest(HitTestState::Enabled);
        layoutMode(LayoutMode::Horizontal);

        if (flags.test(WindowFeatureFlagBit::ToolWindow))
            addStyleClass("mini"_id);

        {
            auto appIcon = createNamedChild<Image>("AppIcon"_id);
            appIcon->hitTest(true);
            appIcon->customStyle("windowAreaType"_id, base::input::AreaType::SysMenu);
        }

        {
            m_title = createNamedChild<TextLabel>("Caption"_id, title);
        }

        if (flags.test(WindowFeatureFlagBit::CanClose) || flags.test(WindowFeatureFlagBit::CanMaximize) || flags.test(WindowFeatureFlagBit::CanMinimize))
        {
            auto buttonCluster = createNamedChild<>("ButtonContainer"_id);
            buttonCluster->layoutHorizontal();

            if (flags.test(WindowFeatureFlagBit::CanMinimize))
            {
                auto minimizeButton = buttonCluster->createNamedChild<Button>("Minimize"_id);
                minimizeButton->attachChild(base::RefNew<TextLabel>());
                minimizeButton->bind(EVENT_CLICKED) = [this]()
                {
                    if (auto window = findParentWindow())
                        window->requestMinimize();
                };
            }

            if (flags.test(WindowFeatureFlagBit::CanMaximize))
            {
                auto maximizeButton = buttonCluster->createNamedChild<Button>("Maximize"_id);
                maximizeButton->attachChild(base::RefNew<TextLabel>());
                maximizeButton->bind(EVENT_CLICKED) = [this]()
                {
                    if (auto window = findParentWindow())
                        window->requestMaximize();
                };
            }

            if (flags.test(WindowFeatureFlagBit::CanClose))
            {
                auto closeButton = buttonCluster->createNamedChild<Button>("Close"_id);
                closeButton->attachChild(base::RefNew<TextLabel>());
                closeButton->bind(EVENT_CLICKED) = [this]()
                {
                    if (auto window = findParentWindow())
                        window->handleExternalCloseRequest();
                };
            }
        }
    }

    bool WindowTitleBar::handleWindowAreaQuery(const ElementArea& area, const Position& absolutePosition, base::input::AreaType& outAreaType) const
    {
        outAreaType = base::input::AreaType::Caption;
        return true;
    }

    //--

} // ui