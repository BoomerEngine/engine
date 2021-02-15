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
#include "renderingSceneObjects.h"
#include "renderingSceneObjectManager.h"

#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceService.h"

namespace rendering
{
    namespace scene
    {
        //----

        base::ConfigProperty<uint32_t> cvRenderSceneMaxObject("Rendering.Scene", "MaxObjects", 128 * 1024);
        base::ConfigProperty<uint32_t> cvRenderSceneMaxObjectEditor("Rendering.Scene", "MaxObjectsEditor", 128 * 1024);
        base::ConfigProperty<uint32_t> cvRenderSceneMaxObjectPreview("Rendering.Scene", "MaxObjectsPreview", 64);

		//----

		ICommandDispatcher::~ICommandDispatcher()
		{}

#define RENDER_SCENE_COMMAND(x) void ICommandDispatcher::run(const Command##x& op) {};
#include "renderingSceneCommands.inl"
#include "../../device/include/renderingDeviceService.h"
#include "../../material/include/renderingMaterialRuntimeService.h"

        //----

        Scene::Scene(SceneType type)
            : m_type(type)
		{
			m_device = base::GetService<DeviceService>()->device();
			createManagers();
        }

        Scene::~Scene()
        {
			destroyManagers();
        }

		void Scene::createManagers()
		{
			base::InplaceArray<base::SpecificClassType<IObjectManager>, 10> objectManagersClasses;
			RTTI::GetInstance().enumClasses(objectManagersClasses);

			static auto* materialService = base::GetService<MaterialService>();

			for (const auto cls : objectManagersClasses)
			{
				auto manager = cls->createPointer<IObjectManager>();
				manager->initialize(this, m_device);

				const auto index = (int)manager->objectType();
				m_managers.prepareWith(index+1, nullptr);
				m_managers[index] = manager;

				materialService->registerMaterialProxyChangeListener(manager);
			}
		}

		void Scene::destroyManagers()
		{
			static auto* materialService = base::GetService<MaterialService>();

			for (auto* manager : m_managers)
			{
				materialService->unregisterMaterialProxyChangeListener(manager);
				delete manager;
			}

			m_managers.clear();
		}

		// TODO: reduce allocation burden

		void Scene::attachProxy(IObjectProxy* proxy)
		{
			auto* cmd = new CommandAttachObject;
			cmd->proxy = AddRef(proxy);
			scheduleCommand(cmd);
		}

		void Scene::dettachProxy(IObjectProxy* proxy)
		{
			auto* cmd = new CommandDetachObject;
			cmd->proxy = AddRef(proxy);
			scheduleCommand(cmd);
		}

		void Scene::moveProxy(IObjectProxy* proxy, const base::Matrix& localToWorld)
		{
            auto* cmd = new CommandMoveObject;
            cmd->proxy = AddRef(proxy);
			cmd->localToWorld = localToWorld;
            scheduleCommand(cmd);
		}

		void Scene::changeProxyFlags(IObjectProxy* proxy, ObjectProxyFlags clearFlag, ObjectProxyFlags setFlag)
		{
            auto* cmd = new CommandChangeFlags;
            cmd->proxy = AddRef(proxy);
			cmd->clearFlags = clearFlag;
			cmd->setFlags = setFlag;
            scheduleCommand(cmd);
		}

        //--

		void Scene::renderLock()
		{
			const auto prevFlag = m_commandQueueSceneLockedFlag.exchange(true);
			ASSERT_EX(prevFlag == false, "Scene already locked for rendering");
		}

		void Scene::renderUnlock()
		{
			const auto prevFlag = m_commandQueueSceneLockedFlag.exchange(false);
			ASSERT_EX(prevFlag == true, "Scene was not locked for rendering");
		}

		void Scene::renderMainView(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame)
		{
			for (auto* manager : m_managers)
				if (manager)
					manager->render(cmd, view, frame);
		}

		void Scene::renderCascadesView(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame)
		{
            for (auto* manager : m_managers)
                if (manager)
                    manager->render(cmd, view, frame);
		}

		void Scene::renderWireframeView(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame)
		{
            for (auto* manager : m_managers)
                if (manager)
                    manager->render(cmd, view, frame);
		}

		void Scene::renderCaptureSelectionView(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame)
		{
            for (auto* manager : m_managers)
                if (manager)
                    manager->render(cmd, view, frame);
		}

		void Scene::renderCaptureDepthView(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame)
		{
            for (auto* manager : m_managers)
                if (manager)
                    manager->render(cmd, view, frame);
		}

		void Scene::prepare(command::CommandWriter& cmd, const FrameRenderer& frame)
		{
			for (auto* manager : m_managers)
				if (manager)
					manager->prepare(cmd, m_device, frame);
		}

		void Scene::scheduleCommand(Command* command)
		{
			auto lock = CreateLock(m_commandQueueLock);

			if (m_commandQueueSceneLockedFlag.load())
			{
				m_commandQueue.push(command);
			}
			else
			{
				runCommand(command);
			}
		}

		Command::~Command()
		{}

		void Scene::runCommand(Command* command)
		{
			const auto* proxy = static_cast<const IObjectProxyBase*>(command->proxy.get());

			auto* manager = m_managers[(int)proxy->m_type];
			command->run(*manager);
		}

		//--

    } // scene
} // rendering
