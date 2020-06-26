/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
* [# filter: game #]
***/

#pragma once

namespace game
{
    //--

    // setup for rendering of the game screen
    struct HostViewport
    {
        // NOTE: may change
        uint32_t width = 0;
        uint32_t height = 0;
        rendering::ImageView backBufferColor;
        rendering::ImageView backBufferDepth;
    };

    //--

    // type of host
    enum class HostType : uint8_t
    {
        Standalone, // standalone, actual game
        InEditor, // in-editor game
        Server, // headless server
    };

    //--

    /// Container class for master game project, worlds, scenes and everything else
    /// Can be instanced as a standalone app/project or in editor
    /// This class also provides the highest (global) level of scripting in the game - allows to write stuff like loading screens, settings panel, world selection etc
    /// This class is NOT intended to be derived from, internal flow is managed with "game screens"
    class GAME_HOST_API Host : public base::IObject
    {
        RTTI_DECLARE_VIRTUAL_CLASS(Host, base::IObject);

    public:
        Host(HostType type, const ScreenPtr& initialScreen);
        virtual ~Host();

        //-- 

        // get type of the game host
        INLINE HostType type() const { return m_type; }

        //--

        // update with given engine time delta, returns false if there are no screens to udpate (game finished)
        bool update(double dt);

        // render all required content, all command should be recorded via provided command buffer writer
        void render(rendering::command::CommandWriter& cmd, const HostViewport& viewport);

        // service input message
        bool input(const base::input::BaseEvent& evt);

        //--

        // post external even to the game, can happen from anywhere (threadsafe)
        void postExternalEvent(const EventPtr& evt);

    private:
        base::Array<ScreenPtr> m_stack;
        HostType m_type;

        base::Array<EventSupplierPtr> m_eventSuppliers;

        base::SpinLock m_externalEventLock;
        base::Array<EventPtr> m_externalEvents;

        base::UniquePtr<base::debug::DebugPageContainer> m_debugPages;

        //--

        void resolveTransition(base::Array<ScreenPtr>& stack, uint32_t level, const ScreenTransitionRequest& transition) const;
        void applyNewStack(const base::Array<ScreenPtr>& newStack);

        //--

        void createEventSuppliers();
        void createDebugPages();
    };

    //--

} // game
