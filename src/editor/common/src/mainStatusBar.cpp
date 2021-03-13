/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: common #]
***/

#include "build.h"
#include "task.h"
#include "service.h"
#include "mainStatusBar.h"

#include "engine/ui/include/uiButton.h"
#include "engine/ui/include/uiProgressBar.h"
#include "engine/ui/include/uiTextLabel.h"

BEGIN_BOOMER_NAMESPACE_EX(ed)

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(MainStatusBar);
    RTTI_METADATA(ui::ElementClassNameMetadata).name("StatusBar");
RTTI_END_TYPE();

MainStatusBar::MainStatusBar()
    : m_taskEvents(this)
{
    layoutHorizontal();

    {
        auto regionLeft = createChild<ui::IElement>();
        regionLeft->customHorizontalAligment(ui::ElementHorizontalLayout::Left);
        regionLeft->customVerticalAligment(ui::ElementVerticalLayout::Middle);

        //--

        auto button = regionLeft->createChild<ui::Button>(ui::ButtonModeBit::EventOnClick);
        button->bind(ui::EVENT_CLICKED) = [this]() { cmdShowJobDetails(); };

        auto inner = button->createChild<ui::IElement>();
        inner->layoutHorizontal();
        inner->customHorizontalAligment(ui::ElementHorizontalLayout::Center);
        inner->customVerticalAligment(ui::ElementVerticalLayout::Middle);

        m_backgroundJobStatus = inner->createChild<ui::TextLabel>("[img:tick]");
        m_backgroundJobStatus->customVerticalAligment(ui::ElementVerticalLayout::Middle);
        m_backgroundJobStatus->customMargins(5, 0, 5, 0);
        m_backgroundJobStatus->text("[img:valid] Ready");

        m_backgroundJobProgress = inner->createChild<ui::ProgressBar>(true);
        m_backgroundJobProgress->customVerticalAligment(ui::ElementVerticalLayout::Middle);
        m_backgroundJobProgress->customMinSize(700, 10);
        m_backgroundJobProgress->customMargins(5, 0, 5, 0);
        m_backgroundJobProgress->position(1.0f, "Done");
        m_backgroundJobProgress->visibility(false);
    }

    {
        auto regionCenter = createChild<ui::IElement>();
        regionCenter->customHorizontalAligment(ui::ElementHorizontalLayout::Expand);
        regionCenter->customVerticalAligment(ui::ElementVerticalLayout::Middle);
        regionCenter->customProportion(1.0f);
    }

    {
        auto regionRight = createChild<ui::IElement>();
        regionRight->customHorizontalAligment(ui::ElementHorizontalLayout::Right);
        regionRight->customVerticalAligment(ui::ElementVerticalLayout::Middle);           
    }

    {
        m_taskEvents.bind(GetService<EditorService>()->eventKey(), EVENT_EDITOR_TASK_STARTED) = [this](EditorTaskPtr task)
        {
            handleJobStarted(task);
        };

        m_taskEvents.bind(GetService<EditorService>()->eventKey(), EVENT_EDITOR_TASK_FINISHED) = [this](EditorTaskPtr task)
        {
            handleJobFinished(task);
        };
    }
}

void MainStatusBar::handleJobStarted(EditorTaskPtr task)
{
    if (task)
    {
        m_activeBackgroundJob = task;
        m_backgroundJobStatus->text(TempString("[img:hourglass] {}", task->description()));
        m_backgroundJobProgress->visibility(false);

        m_taskEvents.bind(task->eventKey(), EVENT_EDITOR_TASK_PROGRESS) 
            = [this](EditorTaskProgress progress) { handleJobProgress(progress); };

        cmdShowJobDetails();
    }
}

void MainStatusBar::handleJobFinished(EditorTaskPtr task)
{
    m_backgroundJobStatus->text("[img:valid] Ready");
    m_backgroundJobProgress->position(1.0f, "Done");
    m_backgroundJobProgress->visibility(false);

    m_taskEvents.unbind(EVENT_EDITOR_TASK_PROGRESS);

    m_activeBackgroundJob.reset();
}

void MainStatusBar::update()
{
    // manage the job details
    if (m_pendingBackgroundJobUIRequest)
    {
        auto job = std::move(m_pendingBackgroundJobUIRequest);
        m_pendingBackgroundJobUIRequest.reset();

        if (!m_pendingBackgroundJobUIOpenedDialogRequest)
        {
            if (auto dialog = job->fetchDetailsDialog())
            {
                m_pendingBackgroundJobUIOpenedDialogRequest = job;

                auto window = RefNew<ui::Window>(ui::WindowFeatureFlagBit::DEFAULT_DIALOG_RESIZABLE, TempString("Details of {}", job->description()));
                auto windowRef = window.get();

                window->attachChild(dialog);
                dialog->expand();

                auto buttons = window->createChild<ui::IElement>();
                buttons->layoutHorizontal();
                buttons->customPadding(5);
                buttons->customHorizontalAligment(ui::ElementHorizontalLayout::Right);

                {
                    auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "[img:cancel] Cancel").get();
                    button->enable(!job->canceled());
                    button->addStyleClass("red"_id);
                    button->bind(ui::EVENT_CLICKED) = [dialog, job, windowRef, button]()
                    {
                        job->requestCancel();
                        button->enable(false);
                    };
                }

                {
                    auto button = buttons->createChildWithType<ui::Button>("PushButton"_id, "Continue in background");
                    button->bind(ui::EVENT_CLICKED) = [windowRef]() {
                        windowRef->requestClose();
                    };
                }

                window->runModal(this);

                m_pendingBackgroundJobUIOpenedDialogRequest.reset();
            }
        }
    }
}

void MainStatusBar::handleJobProgress(EditorTaskProgress progress)
{

}

void MainStatusBar::cmdCancelBackgroundJob()
{

}

void MainStatusBar::cmdShowJobDetails()
{
    if (m_activeBackgroundJob)
    {
        if (auto window = m_activeBackgroundJob->fetchDetailsDialog())
        {
            m_pendingBackgroundJobUIRequest = m_activeBackgroundJob;
        }
    }
}

//---

END_BOOMER_NAMESPACE_EX(ed)

