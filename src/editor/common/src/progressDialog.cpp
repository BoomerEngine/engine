/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "progressDialog.h"

#include "engine/ui/include/uiRenderer.h"
#include "engine/ui/include/uiElement.h"
#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiTextLabel.h"
#include "engine/ui/include/uiProgressBar.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(ProgressDialog);
RTTI_END_TYPE();

ProgressDialog::ProgressDialog(StringView title, bool canCancel, bool keepAround)
    : ui::Window(ui::WindowFeatureFlagBit::DEFAULT_DIALOG, title)
{
    layoutVertical();

    //actions().bindCommand("Cancel"_id) = [windowRef]() { windowRef->requestClose(0); };
    //actions().bindShortcut("Cancel"_id, "Escape");

    m_progressText = createChild<ui::TextLabel>("Please wait...");
    m_progressText->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_progressText->customMargins(5, 5, 5, 0);

    m_progressBar = createChild<ui::ProgressBar>();
    m_progressBar->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
    m_progressBar->customMinSize(600, 40);
    m_progressBar->customMargins(5, 5, 5, 0);
    m_progressBar->position(0.0f); /// TODO: "long wait" animation

    m_innerArea = createChild<ui::IElement>();
    m_innerArea->expand();
    m_innerArea->customMargins(5, 5, 5, 0);

    if (canCancel || keepAround)
    {
        auto buttons = createChild();
        buttons->layoutHorizontal();
        buttons->customHorizontalAligment(ui::ElementHorizontalLayout::Right);
        buttons->customMargins(5, 5, 5, 5);

        if (canCancel)
        {
            m_cancelButton = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:cancel] Cancel");
            m_cancelButton->addStyleClass("red"_id);
            m_cancelButton->enable(true);
            m_cancelButton->bind(ui::EVENT_CLICKED) = [this]() {
                m_cancelButton->enable(false);
                cmdCancel();
            };
        }

        if (keepAround)
        {
            m_closeButton = buttons->createChildWithType<ui::Button>("PushButton"_id, "Close");
            m_closeButton->enable(false);
            m_closeButton->bind(ui::EVENT_CLICKED) = [this]() {
                m_closeButton->enable(false);
                cmdClose();
            };
        }
    }
}

ProgressDialog::~ProgressDialog()
{
}

void ProgressDialog::signalCanceled()
{
    if (!m_cancelFlag.exchange(true))
    {
        runSync<ProgressDialog>([](ProgressDialog& dlg) {
            if (dlg.m_cancelButton)
                dlg.m_cancelButton->enable(false);
            });
    }
}

void ProgressDialog::signalFinished()
{
    runSync<ProgressDialog>([](ProgressDialog& dlg)
        {
            if (dlg.m_closeButton)
                dlg.m_closeButton->enable(true);
            else
                dlg.requestClose(0);
        });
}

void ProgressDialog::cmdCancel()
{
    m_cancelFlag = true;
}

void ProgressDialog::cmdClose()
{
    requestClose(0);
}

bool ProgressDialog::checkCancelation() const
{
    return m_cancelFlag;
}

void ProgressDialog::reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text)
{
    auto txt = StringBuf(text);

    runSync<ProgressDialog>([currentCount, totalCount, txt](ProgressDialog& dlg)
        {
            dlg.m_progressBar->position(totalCount ? (currentCount / (float)totalCount) : 1.0f);
            dlg.m_progressText->text(txt);
        });
}

//--

END_BOOMER_NAMESPACE_EX(ed)

