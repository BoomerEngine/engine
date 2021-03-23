/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

#include "stats.h"

#include "core/containers/include/refcounted.h"
#include "core/containers/include/staticStructurePool.h"
#include "core/containers/include/queue.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///--

// scene type
enum class RenderingSceneType : uint8_t
{
    Game, // actual full blown game scene, usually there's only one 
    EditorGame, // in-editor game scene, some features may have bigger budgets
    EditorPreview, // in-editor small preview scene
};

///--

// shared object flags
enum class RenderingObjectFlagBit : uint8_t
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

typedef BitFlagsBase<RenderingObjectFlagBit, uint32_t> RenderingObjectFlags;

///--

/// public wrapper for the object
class ENGINE_RENDERING_API IRenderingObject : public IReferencable
{
	RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IRenderingObject);
	RTTI_DECLARE_POOL(POOL_RENDERING_PROXY);

public:
	virtual ~IRenderingObject();
};

///--

// a rendering scene - container for objects of various sorts
class ENGINE_RENDERING_API RenderingScene : public IReferencable
{
public:
    RenderingScene(RenderingSceneType type);
	virtual ~RenderingScene();

    //--

    // type of the scene
    INLINE const RenderingSceneType type() const { return m_type; }

	// get object manager
	template <typename T>
	INLINE T* manager() const
	{
		static_assert(std::is_base_of<IRenderingObjectManager, T>::value, "Only manager classes are supported");
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
    RenderingSceneType m_type = RenderingSceneType::Game;

	//--

	std::atomic<bool> m_renderLockFlag = false;

	Array<IRenderingObjectManager*> m_managers;

	//--

	void createManagers();
	void destroyManagers();

	//--

	void renderMainView(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame) const;
	void renderCascadesView(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame) const;
	void renderWireframeView(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame) const;
	void renderCaptureSelectionView(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame) const;
	void renderCaptureDepthView(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame) const;

	void prepare(gpu::CommandWriter& cmd, const FrameRenderer& frame);
	void finish(gpu::CommandWriter& cmd, const FrameRenderer& frame, FrameStats& outStats);

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
