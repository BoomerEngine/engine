/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneObject.h"

#include "gpu/device/include/renderingCommandBuffer.h"
#include "gpu/device/include/renderingDeviceService.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//----

ConfigProperty<uint32_t> cvRenderSceneMaxObject("Rendering.Scene", "MaxObjects", 128 * 1024);
ConfigProperty<uint32_t> cvRenderSceneMaxObjectEditor("Rendering.Scene", "MaxObjectsEditor", 128 * 1024);
ConfigProperty<uint32_t> cvRenderSceneMaxObjectPreview("Rendering.Scene", "MaxObjectsPreview", 64);

//----

Scene::Scene(SceneType type)
    : m_type(type)
{
	m_device = GetService<DeviceService>()->device();
	createManagers();
}

Scene::~Scene()
{
	destroyManagers();
}

void Scene::createManagers()
{
	InplaceArray<SpecificClassType<IObjectManager>, 10> objectManagersClasses;
	RTTI::GetInstance().enumClasses(objectManagersClasses);

	for (const auto cls : objectManagersClasses)
	{
		auto manager = cls->createPointer<IObjectManager>();
		manager->initialize(this, m_device);

		const auto index = m_managers.size();
		m_managers.pushBack(manager);

		cls->assignUserIndex(index);
	}
}

void Scene::destroyManagers()
{
	for (auto* manager : m_managers)
		manager->shutdown();

    for (auto* manager : m_managers)
		delete manager;
	
	m_managers.clear();
}

//--

void Scene::renderLock()
{
	const auto prevFlag = m_renderLockFlag.exchange(true);
	ASSERT_EX(prevFlag == false, "Scene already locked for rendering");
}

void Scene::renderUnlock()
{
	const auto prevFlag = m_renderLockFlag.exchange(false);
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

void Scene::prepare(gpu::CommandWriter& cmd, const FrameRenderer& frame)
{
	for (auto* manager : m_managers)
		if (manager)
			manager->prepare(cmd, m_device, frame);
}

//--

END_BOOMER_NAMESPACE_EX(rendering)
