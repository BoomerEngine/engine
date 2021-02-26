/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

#include "renderingSceneStats.h"

#include "core/containers/include/refcounted.h"
#include "core/containers/include/staticStructurePool.h"
#include "core/containers/include/queue.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///--

// scene type
enum class SceneType : uint8_t
{
    Game, // actual full blown game scene, usually there's only one 
    EditorGame, // in-editor game scene, some features may have bigger budgets
    EditorPreview, // in-editor small preview scene
};

///--

// shared object flags
enum class ObjectProxyFlagBit : uint8_t
{
	Attached,
	Visible,
	Selected,
	CastShadows,
	ReceivesShadows,
	ForceTwoSided,
	ForceTwoSidedShadows,
	ForceShadowsOnly,
};

typedef BitFlagsBase<ObjectProxyFlagBit, uint32_t> ObjectProxyFlags;

///--

/// public wrapper for the object
class ENGINE_RENDERING_API IObjectProxy : public IReferencable
{
	RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IObjectProxy);
	RTTI_DECLARE_POOL(POOL_RENDERING_PROXY);

public:
	virtual ~IObjectProxy();
};

///--

// a rendering scene - container for objects of various sorts
class ENGINE_RENDERING_API Scene : public IReferencable
{
public:
    Scene(SceneType type);
	virtual ~Scene();

    //--

    // type of the scene
    INLINE const SceneType type() const { return m_type; }

	// get object manager
	template <typename T>
	INLINE T* manager() const
	{
		static_assert(std::is_base_of<IObjectManager, T>::value, "Only manager classes are supported");
		static const auto index = T::GetStaticClass()->userIndex();
		return static_cast<T*>(m_managers[index]);
	}

	//--

	// lock scene for rendering, will wait for previous lock to be lifted
	// NOTE: any updates to the scene are no longer legal after this point
    void renderLock();

	// unlock previous scene lock
    void renderUnlock();

	//--

private:
	gpu::IDevice* m_device = nullptr;
    SceneType m_type = SceneType::Game;

	//--

	std::atomic<bool> m_renderLockFlag = false;

	Array<IObjectManager*> m_managers;

	//--

	void createManagers();
	void destroyManagers();

	//--

	void renderMainView(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame);
	void renderCascadesView(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame);
	void renderWireframeView(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame);
	void renderCaptureSelectionView(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame);
	void renderCaptureDepthView(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame);

	void prepare(gpu::CommandWriter& cmd, const FrameRenderer& frame);

	friend class FrameRenderingService;
	friend class FrameViewMain;
	friend class FrameViewCascades;
	friend class FrameViewWireframe;
	friend class FrameViewSelection;
	friend class FrameViewCaptureSelection;
	friend class FrameViewCaptureDepth;
	friend class FrameRenderer;

	//--
};

///--

END_BOOMER_NAMESPACE_EX(rendering)
