/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#pragma once

#include "renderingSceneObject.h"

#include "core/object/include/objectSelection.h"
#include "engine/material/include/renderingMaterialRuntimeService.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

///---

struct ObjectProxyMeshChunk
{
    MeshChunkProxyPtr data;
    MaterialDataProxyPtr material;
    MaterialTemplateProxyPtr shader;
    uint16_t materialIndex = 0;
    uint8_t lodMask = 0;
    char depthPassType = -1;
    char forwardPassType = -1;
    bool staticGeometry = false;
    uint32_t renderMask = 0;

    void updatePassTypes();
};

struct ObjectProxyMeshLOD
{
    float minDistance = 0.0f;
    float maxDistance = 0.0f;
};

class ENGINE_RENDERING_API ObjectProxyMesh : public IObjectProxy
{
    RTTI_DECLARE_VIRTUAL_CLASS(ObjectProxyMesh, IObjectProxy);

public:
    ObjectProxyMesh();
    virtual ~ObjectProxyMesh();

    Color m_color = Color::WHITE;

    Color m_colorEx = Color::BLACK;
    //--

    ObjectProxyFlags m_flags;

    uint8_t m_numLods = 0;
    uint16_t m_numChunks = 0;

    Matrix m_localToWorld;
    Box m_localBox;

    Selectable m_selectable;

    //--

    inline const ObjectProxyMeshLOD* lods() const { return (const ObjectProxyMeshLOD*)(this + 1); }
    inline const ObjectProxyMeshChunk* chunks() const { return (const ObjectProxyMeshChunk*)(lods() + m_numLods); }
    inline ObjectProxyMeshLOD* lods() { return (ObjectProxyMeshLOD*)(this + 1); }
    inline ObjectProxyMeshChunk* chunks() { return (ObjectProxyMeshChunk*)(lods() + m_numLods); }

    // WARNING: packed data follows!
    //

    //--

    struct Setup
    {
        char forcedLodLevel = -1;
        bool staticGeometry = true;

        const Mesh* mesh = nullptr;

        const IMaterial* forceMaterial = nullptr;

        HashMap<StringID, const IMaterial*> materialOverrides;
        HashSet<StringID> selectiveMaterialMask;
        HashSet<StringID> excludedMaterialMask;
    };

    // compile a mesh proxy from given setup
    static ObjectProxyMeshPtr Compile(const Setup& setup);
};

///--

// object handler for standalone meshes
class ENGINE_RENDERING_API ObjectManagerMesh : public IObjectManager, public IMaterialDataProxyListener
{
	RTTI_DECLARE_VIRTUAL_CLASS(ObjectManagerMesh, IObjectManager);

public:
	ObjectManagerMesh();
	virtual ~ObjectManagerMesh();

	//--

	virtual void initialize(Scene* scene, gpu::IDevice* dev) override final;
	virtual void shutdown() override final;

	virtual void prepare(gpu::CommandWriter& cmd, gpu::IDevice* dev, const FrameRenderer& frame) override final;
	virtual void render(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame) override final;

	//--

	virtual void handleMaterialProxyChanges(const MaterialDataProxyChangesRegistry& changedProxies) override final;

	//--

    void attachProxy(ObjectProxyMeshPtr mesh);
    void detachProxy(ObjectProxyMeshPtr mesh);
    void moveProxy(ObjectProxyMeshPtr mesh, Matrix newLocation);
	void updateProxyFlag(ObjectProxyMeshPtr mesh, ObjectProxyFlags clearFlags, ObjectProxyFlags setFlags);

	//--

private:
    struct GPUObjectInfo
	{
        Matrix LocalToScene;

        Vector4 SceneBoundsMin;
		Vector4 SceneBoundsMax;

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

	HashMap<ObjectProxyMesh*, LocalObject> m_localObjects;

	struct VisibleChunkList
	{
        Array<VisibleStandaloneChunk> standaloneChunks;

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
	void renderChunkListStandalone(gpu::CommandWriter& cmd, const Array<VisibleStandaloneChunk>& chunks, MaterialPass pass) const;

	void sortChunksByBatch(Array<VisibleStandaloneChunk>& chunks) const;

	//--
};

///---

END_BOOMER_NAMESPACE_EX(rendering)
