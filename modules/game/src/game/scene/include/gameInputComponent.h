/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: entity\components #]
*
***/

#pragma once

#include "gameComponent.h"

namespace game
{
    //--

    // A scene component that processes raw input and produces game input
    // Component can be activated with activate()/deactivate() methods and when active will produce input for it's entity (passes via OnGameInput)
    class GAME_SCENE_API InputComponent : public Component
    {
        RTTI_DECLARE_VIRTUAL_CLASS(InputComponent, Component);

    public:
        InputComponent();
        InputComponent(const InputDefinitionsPtr& defs);
        virtual ~InputComponent();

        ///--

        /// exclusive ? (if so no input will get past this component) most of the input is like that
        INLINE bool exclusive() const { return m_exclusive; }

        /// priority index in the input stack
        INLINE int priority() const { return m_priority; }

        ///--

        /// activate this input component (it will start receiving input from the user)
        /// NOTE: does not work if we are not attached to world
        void activate();

        /// deactivate this input component (will be removed from stack)
        void deactivate();

        ///--

        /// process input event, can generate and pass events to the entity
        virtual bool handleInput(const base::input::BaseEvent& evt) const;

    protected:
        InputDefinitionsRef m_definitions;
        base::StringBuf m_initialContextName;

        InputContextPtr m_context;

        bool m_active = false;

        bool m_exclusive = false;
        int m_priority = 0;

        //--

        // Component
        virtual void handleAttach(World* scene) override;
        virtual void handleDetach(World* scene) override;
    };

    //--

} // game
