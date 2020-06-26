/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: object #]
***/

#pragma once

namespace base
{
    //--

    /// event callback function, called when object posts a change
    typedef std::function<void(OBJECT_EVENT_FUNC)> TObjectEventFunction;

    //--

    /// Global tracker of changes to objects, expensive and does lots of locks so main purpose is editor like functionality
    class BASE_OBJECT_API IObjectObserver : public base::NoCopy
    {
    public:
        IObjectObserver();
        virtual ~IObjectObserver();

        /// event happened on object, called only if object we observing still exists
        /// NOTE: object is guaranteed to be alive for the duration of this call, but no longer
        /// NOTE: can be called right from within the IObject::notifyObjectChanged() but most often it will be called from a main thread
        virtual void onObjectChangedEvent(StringID eventID, const IObject* eventObject, StringView<char> eventPath, const rtti::DataHolder& eventData) = 0;

        //--

        // called when listener is registered into tracker
        // DEBUG FUNCTIONALITY
        void addTrackingReference();

        // called when listener is unregistered from tracker
        // DEBUG FUNCTIONALITY
        void removeTrackingReference();

        //--

        /// dispatch all pending object observer events
        static void DispatchPendingEvents();

        /// register observer for object
        static uint32_t RegisterObserver(const IObject* obj, IObjectObserver* observer);

        /// unregister previously registered observer for object
        static void UnregisterObserver(uint32_t id, IObjectObserver* observer);

    private:
        std::atomic<uint32_t> registrationCount = 0;
    };

    //--

    /// helper class to embed listener as a member of a class
    /// NOTE: NOT COPIABLE! and NOT to be used on threads different than main thread
    class BASE_OBJECT_API ObjectObserver : public IObjectObserver
    {
    public:
        ObjectObserver(StringID eventName = StringID());
        ~ObjectObserver();

        /// whatever we were bound to unbind from it
        void unbind();

        /// bind to object ID, unbinds previous bindings
        void bindObject(IObject* object);

        /// set callback function
        void bindCallback(const TObjectEventFunction& func);

    private:
        StringID observedEvent;
        uint32_t observedObjectId = 0;

        TObjectEventFunction callback;

        virtual void onObjectChangedEvent(OBJECT_EVENT_FUNC) override final;
    };

} // base
