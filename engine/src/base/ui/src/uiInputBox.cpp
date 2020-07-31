/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: modal #]
***/

#include "build.h"
#include "uiInputBox.h"
#include "uiButton.h"
#include "uiTextLabel.h"
#include "uiEditBox.h"
#include "uiWindow.h"

#include "base/containers/include/stringBuilder.h"

namespace ui
{
    //---

    InputBoxSetup::InputBoxSetup()
        : m_title("Input text")
    {}

    //---

    bool ShowInputBox(IElement* owner, const InputBoxSetup& setup, base::StringBuf& inOutText)
    {
        auto window = base::CreateSharedPtr<Window>(WindowFeatureFlagBit::DEFAULT_DIALOG, setup.m_title);
        window->layoutVertical();

        auto windowRef = window.get();
        window->actions().bindCommand("Cancel"_id) = [windowRef]() { windowRef->requestClose(0); };
        window->actions().bindShortcut("Cancel"_id, "Escape");

        if (setup.m_message)
            window->createChild<TextLabel>(setup.m_message);

        EditBoxFeatureFlags editBoxFlags;
        if (setup.m_multiline)
            editBoxFlags |= EditBoxFeatureBit::Multiline;

        auto editText = window->createChild<EditBox>(editBoxFlags).get();
        if (setup.m_multiline)
            editText->customInitialSize(500, 400);
        else
            editText->customInitialSize(500, 20);
        editText->customMargins(5, 5, 5, 5);
        editText->text(inOutText);
        editText->expand();

        auto buttons = window->createChild();
        buttons->layoutHorizontal();
        buttons->customHorizontalAligment(ElementHorizontalLayout::Right);

        {
            auto button = buttons->createChildWithType<Button>("PushButton"_id, "OK").get();
            button->bind(EVENT_CLICKED) = [windowRef, editText, &inOutText]() {
                if (editText->validationResult()) {
                    inOutText = editText->text();
                    windowRef->requestClose(1);
                }
            };

            editText->bind(EVENT_TEXT_VALIDATION_CHANGED) = [editText, button](bool valid)
            {
                button->enable(valid);
            };
        }

        {
            auto button = buttons->createChildWithType<Button>("PushButton"_id, "Cancel");
            button->bind(EVENT_CLICKED) = [windowRef]() {
                windowRef->requestClose(0);
            };
        }

        if (setup.m_validation)
            editText->validation(setup.m_validation);

        return window->runModal(owner);
    }

    //---

} // ui

