/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#pragma once

#include "engine/ui/include/uiWindow.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

///---

/// basic progress update dialog
class EDITOR_COMMON_API ProgressDialog : public ui::Window, public IProgressTracker
{
    RTTI_DECLARE_VIRTUAL_CLASS(ProgressDialog, ui::Window);

public:
    ProgressDialog(StringView title, bool canCancel = false, bool keepAround = false);
    virtual ~ProgressDialog();

    inline bool keepAround() const { return !m_closeButton.empty(); }

    void signalFinished(); // unlocks the "close" button or closes the dialog
    void signalCanceled(); // signals external cancel request

protected:
    ui::ElementPtr m_innerArea;

private:
    ui::ProgressBarPtr m_progressBar;
    ui::TextLabelPtr m_progressText;

    ui::ButtonPtr m_cancelButton;
    ui::ButtonPtr m_closeButton;

    std::atomic<bool> m_cancelFlag = false;

    virtual void cmdCancel();
    virtual void cmdClose();

    virtual bool checkCancelation() const override final;
    virtual void reportProgress(uint64_t currentCount, uint64_t totalCount, StringView text) override final;
};

///---

END_BOOMER_NAMESPACE_EX(ed)

