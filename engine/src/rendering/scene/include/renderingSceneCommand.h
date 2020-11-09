/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

namespace rendering
{
    namespace scene
    {
        //--

        // command code
        enum class CommandCode : uint8_t
        {
#define RENDER_SCENE_COMMAND(x) x,
#include "renderingSceneCommandCodes.inl"
#undef RENDER_SCENE_COMMAND
            MAX,
        };

        ///--

        /// a command to be executed on the proxy
        struct Command
        {
        public:
            INLINE Command(CommandCode code)
                : m_commandCode(code)
            {}

            INLINE CommandCode code() const
            {
                return m_commandCode;
            }

        private:
            CommandCode m_commandCode;
        };

        ///--

        /// a command to be executed on the proxy
        template< CommandCode opcode >
        struct CommandT : public Command
        {
            static const CommandCode CODE = opcode;

            INLINE CommandT() 
                : Command(CODE)
            {}
        };

        //--

#define RENDER_DECLARE_COMMAND_DATA(op_) struct RENDERING_SCENE_API Command##op_ : public CommandT<CommandCode::op_>

        RENDER_DECLARE_COMMAND_DATA(Nop)
        {
        };

        RENDER_DECLARE_COMMAND_DATA(MoveProxy)
        {
            base::Matrix localToScene;
        };

        RENDER_DECLARE_COMMAND_DATA(EffectSelectionHighlight)
        {
            bool flag = false;
        };

#undef RENDER_DECLARE_COMMAND_DATA

        ///--

        // command dispatcher interfaces - can accept and execute commands
        class RENDERING_SCENE_API ICommandDispatcher : public base::NoCopy
        {
        public:
            virtual ~ICommandDispatcher();

            // run a command
            void handleCommand(Scene* scene, IProxy* proxy, const Command& cmd);

#define RENDER_SCENE_COMMAND(op_) virtual void run##op_(Scene* scene, IProxy* proxy, const Command##op_& cmd);
#include "renderingSceneCommandCodes.inl"
#undef RENDER_SCENE_COMMAND
        };

        ///--

    } // scene
} // rendering

