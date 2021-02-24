/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\notification #]
***/

#include "build.h"
#include "uiNotificationPanel.h"
#include "uiTextLabel.h"
#include "uiWindow.h"

BEGIN_BOOMER_NAMESPACE(ui)

//--

base::ConfigProperty<float> cvDefaultNotificationTime("Editor.Notification", "DefaultNotificationTime", 5.0f);

//--

RTTI_BEGIN_TYPE_CLASS(NotificationPanel);
    RTTI_METADATA(ElementClassNameMetadata).name("NotificationPanel");
RTTI_END_TYPE();

NotificationPanel::NotificationPanel()
    : m_updateTimer(this, "UpdateNotifications"_id)
{
    layoutVertical();

    m_notificationLastUpdateTime.resetToNow();
    m_updateTimer.startRepeated(0.05f);
    m_updateTimer = [this]() { updateNotification(); };
}

NotificationPanel::~NotificationPanel()
{
    removeAllNotifications();
}

void NotificationPanel::updateNotification()
{
    auto dt = std::min<float>(0.1f, m_notificationLastUpdateTime.timeTillNow().toSeconds());
    m_notificationLastUpdateTime.resetToNow();

    auto oldNotifations = std::move(m_notifications);
    for (auto* n : oldNotifations)
    {
        n->timeLeft -= dt;

        if (n->timeLeft > 0.0f)
        {
            if (!n->attached)
            {
                attachChild(n->wrapper);
                n->attached = true;
            }

            auto alpha = std::min<float>(1.0f, n->timeLeft * 2.0f);
            n->wrapper->opacity(alpha);
            m_notifications.pushBack(n);
        }
        else
        {
            if (n->attached)
            {
                detachChild(n->wrapper);
                n->attached = false;
            }
            delete n;
        }
    }
}

void NotificationPanel::removeAllNotifications()
{
    for (auto* n : m_notifications)
    {
        detachChild(n->wrapper);
        delete n;
    }
    m_notifications.reset();
}

void NotificationPanel::postNotification(base::StringID group, IElement* notification, float visibleFor /*= 0.0f*/, base::StringID additionalFrameStyle)
{
    if (notification)
    {
        if (group.empty())
            group = "Default"_id;

        auto lock = base::CreateLock(m_notificationLock);

        if (visibleFor <= 0.0f)
            visibleFor = cvDefaultNotificationTime.get();

        for (const auto& n : m_notifications)
            if (n->group == group)
                n->timeLeft = 0.0f;

        auto n = new Notificaton;
        n->group = group;
        n->wrapper = base::RefNew<ui::IElement>();
        n->wrapper->layoutVertical();
        n->wrapper->styleType("NotificationFrame"_id);
        n->wrapper->attachChild(notification);

        if (additionalFrameStyle)
            n->wrapper->addStyleClass(additionalFrameStyle);

        n->content = base::RefPtr<IElement>(AddRef(notification));
        n->timeLeft = visibleFor;
        m_notifications.pushBack(n);
    }
}

void NotificationPanel::postTextNotification(base::StringID group, base::StringView txt, MessageType type /*= MessageType::Info*/, float visibleFor /*= 0.0f*/)
{
    if (txt)
    {
        base::StringID frameStyle;
        if (type == MessageType::Error)
            frameStyle = "error"_id;
        else if (type == MessageType::Warning)
            frameStyle = "warning"_id;

        auto data = base::RefNew<TextLabel>(txt);
        postNotification(group, data, visibleFor, frameStyle);
    }
}

void NotificationPanel::postLargeTextNotification(base::StringID group, base::StringView txt, MessageType type /*= MessageType::Info*/, float visibleFor /*= 0.0f*/)
{

}

//--

void PostWindowMessage(IElement* owner, MessageType type, base::StringID group, base::StringView txt)
{
    auto safeOwner = base::RefWeakPtr<IElement>(owner);
    auto safeText = base::StringBuf(txt);

    RunSync("NotifyWindow") << [safeOwner, type, group, safeText](FIBER_FUNC)
    {
        if (auto owner = safeOwner.lock())
        {
            if (auto* window = owner->findParentWindow())
            {
                auto notificationPanel = window->findChildByName<NotificationPanel>(base::StringID::EMPTY(), false);
                if (!notificationPanel)
                {
                    notificationPanel = window->createChild<NotificationPanel>();
                    notificationPanel->overlay(true);
                    notificationPanel->customMargins(100, 100, 100, 100);
                    notificationPanel->customHorizontalAligment(ElementHorizontalLayout::Right);
                    notificationPanel->customVerticalAligment(ElementVerticalLayout::Bottom);
                }

                notificationPanel->postTextNotification(group, safeText, type);
            }
        }
    };
}

void PostNotificationMessage(IElement* owner, MessageType type, base::StringID group, base::StringView txt)
{
    auto safeOwner = base::RefWeakPtr<IElement>(owner);
    auto safeText = base::StringBuf(txt);

    RunSync("NotifyWindow") << [safeOwner, type, group, safeText](FIBER_FUNC)
    {
        if (auto ownerPtr = safeOwner.lock())
        {
            auto owner = ownerPtr.get();
            while (owner)
            {
                if (auto notificationPanel = owner->findChildByName<NotificationPanel>(base::StringID::EMPTY(), false))
                    notificationPanel->postTextNotification(group, safeText, type);

                owner = owner->parentElement();
            }
        }
    };
}

//--

END_BOOMER_NAMESPACE(ui)