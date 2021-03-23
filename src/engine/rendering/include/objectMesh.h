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

struct RenderingObjectMeshChunk
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

struct RenderingObjectMeshLOD
{
    float minDistanceSquared = 0.0f;
    float maxDistanceSquared = 0.0f;
};

struct RenderingMeshStats
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

struct RenderingMeshVisibilityStats
{
    uint32_t numTestedObjects = 0;
    uint32_t numVisibleObjects = 0;
    uint32_t numTestedChunks = 0;
    uint32_t numVisibleChunks = 0;
    double cullingTime = 0.0;
};

struct RenderingMeshTotalStats
{
    RenderingMeshVisibilityStats mainVisibility;
    RenderingMeshVisibilityStats globalShadowsVisibility;

    RenderingMeshStats depthBatching;
    RenderingMeshStats mainBatching;
    RenderingMeshStats globalShadowsBatching;
    RenderingMeshStats localShadowsBatching;
    double totalTime = 0.0;
};

class ENGINE_RENDERING_API RenderingMesh : public IRenderingObject
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingMesh, IRenderingObject);

public:
    RenderingMesh();
    virtual ~RenderingMesh();

    Color m_color = Color::WHITE;

    Color m_colorEx = Color::BLACK;
    //--

    RenderingObjectFlags m_flags;

    uint8_t m_numLods = 0;
    uint16_t m_numChunks = 0;

    Matrix m_localToWorld;
    Box m_localBox;

    Selectable m_selectable;

    //--

    inline const RenderingObjectMeshLOD* lods() const { return (const RenderingObjectMeshLOD*)(this + 1); }
    inline const RenderingObjectMeshChunk* chunks() const { return (const RenderingObjectMeshChunk*)(lods() + m_numLods); }
    inline RenderingObjectMeshLOD* lods() { return (RenderingObjectMeshLOD*)(this + 1); }
    inline RenderingObjectMeshChunk* chunks() { return (RenderingObjectMeshChunk*)(lods() + m_numLods); }

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
    static RenderingMeshPtr Compile(const Setup& setup);

    //--

    // get total visibility distance
    float visibilityDistanceSquared() const;

    // determine LOD mask for given squared distance
    uint32_t calcDetailMask(float distanceSquared) const;
};

///--

// object handler for standalone meshes
class ENGINE_RENDERING_API RenderingMeshManager : public IRenderingObjectManager, public IMaterialDataProxyListener
{
	RTTI_DECLARE_VIRTUAL_CLASS(RenderingMeshManager, IRenderingObjectManager);

public:
	RenderingMeshManager();
	virtual ~RenderingMeshManager();

	//--

	virtual void initialize(RenderingScene* scene, gpu::IDevice* dev) override final;
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

    virtual void commandAttachProxy(IRenderingObject* object) override;
    virtual void commandDetachProxy(IRenderingObject* object) override;
    virtual void commandMoveProxy(IRenderingObject* object, Matrix newLocation) override;
    virtual void commandUpdateProxyFlag(IRenderingObject* object, RenderingObjectFlags clearFlags, RenderingObjectFlags setFlags) override;

    void commandAttachProxy(RenderingMeshPtr mesh);
    void commandDetachProxy(RenderingMeshPtr mesh);
    void commandMoveProxy(RenderingMeshPtr mesh, Matrix newLocation);
    void commandUpdateProxyFlag(RenderingMeshPtr mesh, RenderingObjectFlags clearFlags, RenderingObjectFlags setFlags);

	//--

    INLINE const RenderingMeshTotalStats& stats() const { return m_lastStats; }

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
		RenderingMeshPtr data = nullptr;
	};

    typedef uint16_t ObjectIndex;

	struct VisibleStandaloneChunk
	{
		const RenderingMesh* object = nullptr;
        const MaterialTemplateProxy* shader = nullptr;
        const MaterialDataProxy* material = nullptr;
		const MeshChunkProxy_Standalone* chunk = nullptr;
		uint16_t materialIndex = 0;
	};

	HashMap<RenderingMesh*, LocalObject> m_localObjects;

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
    RenderingMeshTotalStats m_lastStats;
    RenderingMeshTotalStats m_stats;

	//--

	void collectMainViewChunks(const FrameViewSingleCamera& view, VisibleMainViewCollector& outCollector, RenderingMeshVisibilityStats& outStats) const;
	void collectWireframeViewChunks(const FrameViewSingleCamera& view, VisibleWireframeViewCollector& outCollector) const;
	void collectCaptureChunks(const FrameViewSingleCamera& view, VisibleCaptureCollector& outCollector) const;

	void renderChunkListStandalone(gpu::CommandWriter& cmd, const Array<VisibleStandaloneChunk>& chunks, MaterialPass pass, RenderingMeshStats& outStats) const;

	void sortChunksByBatch(Array<VisibleStandaloneChunk>& chunks) const;

    void exportStats(const RenderingMeshStats& stats, FrameViewStats& outStats) const;
    void exportStats(const RenderingMeshVisibilityStats& stats, FrameViewStats& outStats) const;

	//--
};

///---

END_BOOMER_NAMESPACE_EX(rendering)
