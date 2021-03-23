/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#pragma once

#include "object.h"

#include "core/object/include/objectSelection.h"
#include "engine/material/include/runtimeService.h"

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
    float minDistanceSquared = 0.0f;
    float maxDistanceSquared = 0.0f;
};

struct ObjectMeshBatchingStats
{
    uint32_t numTriangles = 0;
    uint32_t numShaderChanges = 0;
    uint32_t numShaderVariantChanges = 0;
    uint32_t numMaterialChanges = 0;
    uint32_t numGeometryChanges = 0;
    uint32_t numInstances = 0;
    uint32_t numBatches = 0;
    uint32_t numExecutions = 0;
    double recordingTime = 0.0;
};

struct ObjectMeshVisibilityStats
{
    uint32_t numTestedObjects = 0;
    uint32_t numVisibleObjects = 0;
    uint32_t numTestedChunks = 0;
    uint32_t numVisibleChunks = 0;
    double cullingTime = 0.0;
};

struct ObjectMeshTotalStats
{
    ObjectMeshVisibilityStats mainVisibility;
    ObjectMeshVisibilityStats globalShadowsVisibility;

    ObjectMeshBatchingStats depthBatching;
    ObjectMeshBatchingStats mainBatching;
    ObjectMeshBatchingStats globalShadowsBatching;
    ObjectMeshBatchingStats localShadowsBatching;
    double totalTime = 0.0;
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
        int focedSingleChunk = -1;
        bool staticGeometry = true;

        const Mesh* mesh = nullptr;

        const IMaterial* forceMaterial = nullptr;

        HashMap<StringID, const IMaterial*> materialOverrides;
        HashSet<StringID> selectiveMaterialMask;
        HashSet<StringID> excludedMaterialMask;
    };

    // compile a mesh proxy from given setup
    static ObjectProxyMeshPtr Compile(const Setup& setup);

    //--

    // get total visibility distance
    float visibilityDistanceSquared() const;

    // determine LOD mask for given squared distance
    uint32_t calcDetailMask(float distanceSquared) const;
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
    virtual void finish(gpu::CommandWriter& cmd, gpu::IDevice* dev, const FrameRenderer& frame, FrameStats& outStats) override final;

	virtual void render(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame) override final;
	virtual void render(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame) override final;

	//--

	virtual void handleMaterialProxyChanges(const MaterialDataProxyChangesRegistry& changedProxies) override final;

	//--

    virtual void commandAttachProxy(IObjectProxy* object) override;
    virtual void commandDetachProxy(IObjectProxy* object) override;
    virtual void commandMoveProxy(IObjectProxy* object, Matrix newLocation) override;
    virtual void commandUpdateProxyFlag(IObjectProxy* object, ObjectProxyFlags clearFlags, ObjectProxyFlags setFlags) override;

    void commandAttachProxy(ObjectProxyMeshPtr mesh);
    void commandDetachProxy(ObjectProxyMeshPtr mesh);
    void commandMoveProxy(ObjectProxyMeshPtr mesh, Matrix newLocation);
    void commandUpdateProxyFlag(ObjectProxyMeshPtr mesh, ObjectProxyFlags clearFlags, ObjectProxyFlags setFlags);

	//--

    INLINE const ObjectMeshTotalStats& stats() const { return m_lastStats; }

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
        Vector3 distanceRefPoint;
        float maxDistanceSquared = 0.0f;
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

    SpinLock m_statLock;
    ObjectMeshTotalStats m_lastStats;
    ObjectMeshTotalStats m_stats;

	//--

	void collectMainViewChunks(const FrameViewSingleCamera& view, VisibleMainViewCollector& outCollector, ObjectMeshVisibilityStats& outStats) const;
	void collectWireframeViewChunks(const FrameViewSingleCamera& view, VisibleWireframeViewCollector& outCollector) const;
	void collectCaptureChunks(const FrameViewSingleCamera& view, VisibleCaptureCollector& outCollector) const;

	void renderChunkListStandalone(gpu::CommandWriter& cmd, const Array<VisibleStandaloneChunk>& chunks, MaterialPass pass, ObjectMeshBatchingStats& outStats) const;

	void sortChunksByBatch(Array<VisibleStandaloneChunk>& chunks) const;

    void exportStats(const ObjectMeshBatchingStats& stats, FrameViewStats& outStats) const;
    void exportStats(const ObjectMeshVisibilityStats& stats, FrameViewStats& outStats) const;

	//--
};

///---

END_BOOMER_NAMESPACE_EX(rendering)
