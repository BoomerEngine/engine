/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#pragma once

#include "renderingSceneObjectManager.h"
#include "renderingFrameCamera.h"

#include "rendering/material/include/renderingMaterialRuntimeService.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

///--

// object handler for standalone meshes
class ObjectManagerMesh : public IObjectManager
{
	RTTI_DECLARE_VIRTUAL_CLASS(ObjectManagerMesh, IObjectManager);

public:
	ObjectManagerMesh();
	virtual ~ObjectManagerMesh();

	//--

	virtual ObjectType objectType() const { return ObjectType::Mesh; }

	virtual void initialize(Scene* scene, IDevice* dev) override final;
	virtual void shutdown() override final;

	virtual void prepare(GPUCommandWriter& cmd, IDevice* dev, const FrameRenderer& frame) override final;
	virtual void render(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame) override final;

	//--

	virtual void handleMaterialProxyChanges(const MaterialDataProxyChangesRegistry& changedProxies) override final;

	//--

	virtual void run(const CommandAttachObject& op) override final;
	virtual void run(const CommandDetachObject& op) override final;
	virtual void run(const CommandMoveObject& op) override final;
	virtual void run(const CommandChangeFlags& op) override final;

	//--

private:
    MaterialCache* m_materialCache = nullptr;

	struct GPUObjectInfo
	{
        base::Matrix LocalToScene;

        base::Vector4 SceneBoundsMin;
		base::Vector4 SceneBoundsMax;

        uint32_t SelectionObjectID;
		uint32_t SelectionSubObjectID;
		uint32_t _Padding0;
		uint32_t _Padding1;
	};

	struct LocalObject
	{
		VisibilityBox box;
        //uint16_t chunkCount = 0;
		ObjectProxyMeshPtr data = nullptr;
	};

    typedef uint16_t ObjectIndex;

	struct VisibleStandaloneChunk
	{
		const ObjectProxyMesh* object = nullptr;
        const MaterialTemplateProxy* shader = nullptr;
        const MaterialDataProxy* material = nullptr;
		const MeshChunkProxy_Standalone* chunk = nullptr;
		uint16_t materialIndex = 0;
	};

	base::HashMap<ObjectProxyMesh*, LocalObject> m_localObjects;

	struct VisibleChunkList
	{
        base::Array<VisibleStandaloneChunk> standaloneChunks;

		VisibleChunkList();

		void prepare(uint32_t totalChunkCount);
	};

	struct VisibleMainViewCollector
	{
		VisibleChunkList depthLists[2];
		VisibleChunkList forwardLists[3];
		VisibleChunkList selectionOutlineList;

		void prepare(uint32_t totalChunkCount);
	};

    struct VisibleWireframeViewCollector
    {
        VisibleChunkList mainList;
		VisibleChunkList selectionOutlineList;

        void prepare(uint32_t totalChunkCount);
    };

    struct VisibleCaptureCollector
    {
        VisibleChunkList mainList;

        void prepare(uint32_t totalChunkCount);
    };

	VisibleMainViewCollector m_cacheViewMain;
	VisibleWireframeViewCollector m_cacheViewWireframe;
	VisibleCaptureCollector m_cacheCaptureView;

	//--

	void collectMainViewChunks(const FrameViewSingleCamera& view, VisibleMainViewCollector& outCollector) const;
	void collectWireframeViewChunks(const FrameViewSingleCamera& view, VisibleWireframeViewCollector& outCollector) const;
	void collectCaptureChunks(const FrameViewSingleCamera& view, VisibleCaptureCollector& outCollector) const;
	void renderChunkListStandalone(GPUCommandWriter& cmd, const base::Array<VisibleStandaloneChunk>& chunks, MaterialPass pass) const;

	void sortChunksByBatch(base::Array<VisibleStandaloneChunk>& chunks) const;

	//--
};

///---

END_BOOMER_NAMESPACE(rendering::scene)