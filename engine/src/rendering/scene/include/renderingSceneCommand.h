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

		struct Command;
		class ICommandDispatcher;

		//--

#define RENDER_SCENE_COMMAND(x) struct Command##x;
#include "renderingSceneCommands.inl"

		/// dispatcher for update commands
		class RENDERING_SCENE_API ICommandDispatcher : public base::NoCopy
		{
		public:
			virtual ~ICommandDispatcher();

#define RENDER_SCENE_COMMAND(x) virtual void run(const Command##x& op);
#include "renderingSceneCommands.inl"
		};

		//--

		/// a command to be executed on the proxy
		struct Command
		{
		public:
			virtual ~Command();
			ObjectProxyPtr proxy;

			virtual void run(ICommandDispatcher& dispatch) const = 0;
		};

		/// a command to be executed on the proxy
		template< typename T >
		struct CommandT : public Command
		{
		public:
			virtual void run(ICommandDispatcher& dispatch) const override final
			{
				dispatch.run(static_cast<const T&>(*this));
			}
		};

		//--

#define RENDER_SCENE_DECLARE_UPDATE_COMMAND(x) struct Command##x : public CommandT<Command##x>

		//--

		RENDER_SCENE_DECLARE_UPDATE_COMMAND(AttachObject)
		{
		};

		RENDER_SCENE_DECLARE_UPDATE_COMMAND(DetachObject)
		{
		};

		RENDER_SCENE_DECLARE_UPDATE_COMMAND(SetLocalToWorld)
		{
			base::Matrix localToWorld;
		};

		RENDER_SCENE_DECLARE_UPDATE_COMMAND(SetFlag)
		{
			ObjectProxyFlags setFlags;
			ObjectProxyFlags clearFlags;
		};

#undef RENDER_SCENE_DECLARE_UPDATE_COMMAND

		//--

	} // scene
} // rendering

