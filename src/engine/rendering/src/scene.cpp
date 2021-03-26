/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#include "build.h"
#include "scene.h"
#include "object.h"

#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/deviceService.h"

BEGIN_BOOMER_NAMESPACE()

//----

ConfigProperty<uint32_t> cvRenderSceneMaxObject("Rendering.Scene", "MaxObjects", 128 * 1024);
ConfigProperty<uint32_t> cvRenderSceneMaxObjectEditor("Rendering.Scene", "MaxObjectsEditor", 128 * 1024);
ConfigProperty<uint32_t> cvRenderSceneMaxObjectPreview("Rendering.Scene", "MaxObjectsPreview", 64);

//----

RenderingScene::RenderingScene(RenderingSceneType type)
    : m_type(type)
{
	m_device = GetService<DeviceService>()->device();
	createManagers();
}

RenderingScene::~RenderingScene()
{
	destroyManagers();
}

void RenderingScene::createManagers()
{
	InplaceArray<SpecificClassType<IRenderingObjectManager>, 10> objectManagersClasses;
	RTTI::GetInstance().enumClasses(objectManagersClasses);

	for (const auto cls : objectManagersClasses)
	{
		auto manager = cls->createPointer<IRenderingObjectManager>();
		manager->initialize(this, m_device);

		const auto index = m_managers.size();
		m_managers.pushBack(manager);

		cls->assignUserIndex(index);
	}
}

void RenderingScene::destroyManagers()
{
	for (auto* manager : m_managers)
		manager->shutdown();

    for (auto* manager : m_managers)
		delete manager;
	
	m_managers.clear();
}

//--

void RenderingScene::renderLock()
{
	const auto prevFlag = m_renderLockFlag.exchange(true);
	ASSERT_EX(prevFlag == false, "Scene already locked for rendering");
}

void RenderingScene::renderUnlock()
{
	const auto prevFlag = m_renderLockFlag.exchange(false);
	ASSERT_EX(prevFlag == true, "Scene was not locked for rendering");
}

void RenderingScene::renderMainView(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame) const
{
	for (auto* manager : m_managers)
		if (manager)
			manager->render(cmd, view, frame);
}

void RenderingScene::renderCascadesView(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame) const
{
    for (auto* manager : m_managers)
        if (manager)
            manager->render(cmd, view, frame);
}

void RenderingScene::renderWireframeView(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame) const
{
    for (auto* manager : m_managers)
        if (manager)
            manager->render(cmd, view, frame);
}

void RenderingScene::renderCaptureSelectionView(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame) const
{
    for (auto* manager : m_managers)
        if (manager)
            manager->render(cmd, view, frame);
}

void RenderingScene::renderCaptureDepthView(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame) const
{
    for (auto* manager : m_managers)
        if (manager)
            manager->render(cmd, view, frame);
}

void RenderingScene::prepare(gpu::CommandWriter& cmd, const FrameRenderer& frame)
{
	for (auto* manager : m_managers)
		if (manager)
			manager->prepare(cmd, m_device, frame);
}

void RenderingScene::finish(gpu::CommandWriter& cmd, const FrameRenderer& frame, FrameStats& outStats)
{
    for (auto* manager : m_managers)
        if (manager)
            manager->finish(cmd, m_device, frame, outStats);
}

//--

END_BOOMER_NAMESPACE()
