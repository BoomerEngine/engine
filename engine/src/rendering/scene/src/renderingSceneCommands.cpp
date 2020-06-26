/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneCommand.h"

namespace rendering
{
    namespace scene
    {
        //----

        ICommandDispatcher::~ICommandDispatcher()
        {
        }

#define RENDER_SCENE_COMMAND(op_) void ICommandDispatcher::run##op_(Scene* scene, IProxy* proxy, const Command##op_& cmd) { TRACE_ERROR("Missing handler implementation for command '{}'", #op_); }
#include "renderingSceneCommandCodes.inl"
#undef RENDER_SCENE_COMMAND

        void ICommandDispatcher::handleCommand(Scene* scene, IProxy* proxy, const Command& cmd)
        {
            const auto code = cmd.code();

            switch (code)
            {
#define RENDER_SCENE_COMMAND(op_) case CommandCode::##op_: run##op_(scene, proxy, static_cast<const Command##op_&>(cmd)); break;
#include "renderingSceneCommandCodes.inl"
#undef RENDER_SCENE_COMMAND
            }
        }

        //----

    } // scene
} // rendering
