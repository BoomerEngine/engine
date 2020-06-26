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
        , m_caption("Please input text")
    {}

    InputBoxSetup& InputBoxSetup::title(const char* txt)
    {
        m_title = txt;
        return *this;
    }

    InputBoxSetup& InputBoxSetup::message(const char* txt)
    {
        m_caption = txt;
        return *this;
    }

    //---

    class InputBox : public Window
    {
        RTTI_DECLARE_VIRTUAL_CLASS(InputBox, Window);

    public:
        InputBox();

        // get the result of the message box
        INLINE bool result() const { return m_result; }

        // get the text
        INLINE const base::StringBuf& text() const { return m_text; }

        // configure message box
        bool configure(const InputBoxSetup& setup, const base::StringBuf& text);

    private:
        virtual bool previewKeyEvent(const base::input::KeyEvent &evt) override;

        void cmdOK();
        void cmdCancel();

        base::RefPtr<TextLabel> m_message;
        base::RefPtr<EditBox> m_editText;
        base::StringBuf m_text;
        bool m_result;
    };

    InputBox::InputBox()
        : m_result(false)
    {
        hitTest(true);
        layoutVertical();

        createChild<WindowTitleBar>()->addStyleClass("mini"_id);

        m_message = createChild<TextLabel>("Enter your input here:");

        m_editText = createChild<EditBox>();
        m_editText->customInitialSize(500, 400);
        m_editText->customMargins(5, 5, 5, 5);
        m_editText->expand();

        auto buttons = createChild<IElement>();
        buttons->layoutHorizontal();
        buttons->customHorizontalAligment(ElementHorizontalLayout::Right);

        {
            auto button = buttons->createChildWithType<Button>("PushButton"_id, "OK");
            button->bind("OnClick"_id) = [this]() { cmdOK(); };
        }

        {
            auto button = buttons->createChildWithType<Button>("PushButton"_id, "Cancel");
            button->bind("OnClick"_id) = [this]() { cmdCancel(); };
        }
    }

    void InputBox::cmdOK()
    {
        if (m_editText)
            m_text = m_editText->text();
        m_result = true;
        requestClose();
    }

    void InputBox::cmdCancel()
    {
        m_result = false;
        requestClose();
    }

    bool InputBox::previewKeyEvent(const base::input::KeyEvent &evt)
    {
        if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_RETURN)
        {
            cmdOK();
            return true;
        }
        else if (evt.pressed() && evt.keyCode() == base::input::KeyCode::KEY_ESCAPE)
        {
            cmdCancel();
            return true;
        }

        return TBaseClass::previewKeyEvent(evt);
    }

    bool InputBox::configure(const InputBoxSetup& setup, const base::StringBuf& text)
    {
        m_text = text;

        // set message and title
        if (m_message)
            m_message->text(setup.caption());
        if (m_editText)
        {
            m_editText->text(text);
            m_editText->selectWholeText();
        }

        //baseTitle(setup.title());
        return true;
    }

    RTTI_BEGIN_TYPE_CLASS(InputBox);
    RTTI_END_TYPE();

    //---

    bool ShowInputBox(const ElementPtr& parent, const InputBoxSetup& setup, base::StringBuf& text)
    {
        // no parent window, no message :(
        if (!parent || !parent->renderer())
        {
            TRACE_WARNING("Input  box cannot be displayed without context");
            return false;
        }

        // create and configure the message box window
        auto window = base::CreateSharedPtr<InputBox>();
        if (!window->configure(setup, text))
        {
            TRACE_WARNING("Message box cannot be created from given setup");
            return false;
        }

        // show the message box as the modal window
        //parent->renderer()->showModalWindow(window, parent);

        // return the message box result
        if (!window->result())
            return false;

        // get text
        text = window->text();
        return true;
    }

    //---

} // ui

