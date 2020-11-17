/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\notification #]
***/

#pragma once

#include "uiElement.h"

namespace ui
{
    //---

    /// notification panel, displays ordered list of notifications that slowly expire
    class BASE_UI_API NotificationPanel : public IElement
    {
        RTTI_DECLARE_VIRTUAL_CLASS(NotificationPanel, IElement);

    public:
        NotificationPanel();
        virtual ~NotificationPanel();

        //--

        // remove all posted notifications
        void removeAllNotifications();

        // post a general notification, notification is displayed in a frame for a default time duration unless specified otherwise
        void postNotification(base::StringID group, IElement* notification, float visibleFor = 0.0f, base::StringID additionalFrameStyle = base::StringID::EMPTY());

        // post a small text notification
        void postTextNotification(base::StringID group, base::StringView txt, MessageType type = MessageType::Info, float visibleFor = 0.0f);

        // post a large text notification (use message icons)
        void postLargeTextNotification(base::StringID group, base::StringView txt, MessageType type = MessageType::Info, float visibleFor = 0.0f);

        //--

    private:
        struct Notificaton
        {
            base::StringID group;
            float timeLeft = 0.0f;
            ElementPtr content;
            ElementPtr wrapper;
            bool attached = false;
        };

        Timer m_updateTimer;

        base::Array<Notificaton*> m_notifications;
        base::SpinLock m_notificationLock;

        base::NativeTimePoint m_notificationLastUpdateTime;

        void updateNotification();
    };

    //---

} // ui