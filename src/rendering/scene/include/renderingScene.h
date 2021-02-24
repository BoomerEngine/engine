/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#pragma once

#include "base/containers/include/refcounted.h"
#include "base/containers/include/staticStructurePool.h"
#include "renderingSceneStats.h"
#include "base/containers/include/queue.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

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

typedef base::BitFlagsBase<ObjectProxyFlagBit, uint32_t> ObjectProxyFlags;

///--

/// public wrapper for the object
class RENDERING_SCENE_API IObjectProxy : public base::IReferencable
{
	RTTI_DECLARE_VIRTUAL_ROOT_CLASS(IObjectProxy);
	RTTI_DECLARE_POOL(POOL_RENDERING_PROXY);

public:
	virtual ~IObjectProxy();

	virtual ObjectType type() const = 0;
	virtual bool attached() const = 0;
};

///--

// a rendering scene - container for objects of various sorts
class RENDERING_SCENE_API Scene : public base::IReferencable
{
public:
    Scene(SceneType type);
	virtual ~Scene();

    //--

    // type of the scene
    INLINE const SceneType type() const { return m_type; }

    //--
	// COMMANDS: all are buffered

	/// attach proxy to scene, proxy will be rendered on next scene render
	void attachProxy(IObjectProxy* proxy);

	/// detach proxy from scene
	void dettachProxy(IObjectProxy* proxy);

	/// move object to other place
	void moveProxy(IObjectProxy* proxy, const base::Matrix& localToWorld);

	/// toggle proxy flags
	void changeProxyFlags(IObjectProxy* proxy, ObjectProxyFlags clearFlag, ObjectProxyFlags setFlag);

	//--

private:
	//--

	IDevice* m_device = nullptr;

    SceneType m_type = SceneType::Game;

	//--

	std::atomic<bool> m_commandQueueSceneLockedFlag = false;
	base::SpinLock m_commandQueueLock;
	base::Queue<Command*> m_commandQueue;
           
	//--

	void scheduleCommand(Command* command);
	void runCommand(Command* command);

	//--

	base::Array<IObjectManager*> m_managers;

	//--

	void createManagers();
	void destroyManagers();

	//--

	void renderLock();
	void renderUnlock();
	void renderMainView(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame);
	void renderCascadesView(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame);
	void renderWireframeView(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame);
	void renderCaptureSelectionView(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame);
	void renderCaptureDepthView(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame);

	void prepare(GPUCommandWriter& cmd, const FrameRenderer& frame);

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

END_BOOMER_NAMESPACE(rendering::scene)
