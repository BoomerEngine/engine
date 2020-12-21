/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

#include "renderingSelectable.h"
#include "renderingSceneObjects.h"
#include "renderingSceneCommand.h"

#include "rendering/material/include/renderingMaterialRuntimeService.h"

namespace rendering
{
    namespace scene
    {

		///---

		/// manager for rendering objects
		class RENDERING_SCENE_API IObjectManager : public ICommandDispatcher, public IMaterialDataProxyListener
		{
			RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IObjectManager);

		public:
			virtual ~IObjectManager();
			virtual ObjectType objectType() const = 0;

			virtual void initialize(Scene* scene, IDevice* dev) = 0;
			virtual void shutdown() = 0;

			virtual void prepare(command::CommandWriter& cmd, IDevice* dev, const FrameRenderer& frame) = 0;

			virtual void render(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame) = 0;
		};

		//--

    } // scene
} // rendering

