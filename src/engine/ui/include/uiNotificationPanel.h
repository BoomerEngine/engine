/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\notification #]
***/

#pragma once

#include "uiElement.h"

BEGIN_BOOMER_NAMESPACE_EX(ui)

//---

/// notification panel, displays ordered list of notifications that slowly expire
class ENGINE_UI_API NotificationPanel : public IElement
{
    RTTI_DECLARE_VIRTUAL_CLASS(NotificationPanel, IElement);

public:
    NotificationPanel();
    virtual ~NotificationPanel();

    //--

    // remove all posted notifications
    void removeAllNotifications();

    // post a general notification, notification is displayed in a frame for a default time duration unless specified otherwise
    void postNotification(StringID group, IElement* notification, float visibleFor = 0.0f, StringID additionalFrameStyle = StringID::EMPTY());

    // post a small text notification
    void postTextNotification(StringID group, StringView txt, MessageType type = MessageType::Info, float visibleFor = 0.0f);

    // post a large text notification (use message icons)
    void postLargeTextNotification(StringID group, StringView txt, MessageType type = MessageType::Info, float visibleFor = 0.0f);

    //--

private:
    struct Notificaton : public NoCopy
    {
        RTTI_DECLARE_POOL(POOL_UI_OBJECTS)

    public:
        StringID group;
        float timeLeft = 0.0f;
        ElementPtr content;
        ElementPtr wrapper;
        bool attached = false;
    };

    Timer m_updateTimer;

    Array<Notificaton*> m_notifications;
    SpinLock m_notificationLock;

    NativeTimePoint m_notificationLastUpdateTime;

    void updateNotification();
};

//---

END_BOOMER_NAMESPACE_EX(ui)
