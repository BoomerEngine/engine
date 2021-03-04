/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#include "build.h"
#include "objectMesh.h"

#include "viewMain.h"
#include "viewCascades.h"
#include "viewWireframe.h"
#include "viewCaptureSelection.h"
#include "viewCaptureDepth.h"

#include "engine/mesh/include/mesh.h"
#include "engine/mesh/include/chunkProxy.h"
#include "engine/material/include/material.h"
#include "engine/material/include/materialInstance.h"
#include "engine/material/include/runtimeProxy.h"
#include "engine/material/include/runtimeTemplate.h"

#include "gpu/device/include/descriptor.h"

BEGIN_BOOMER_NAMESPACE_EX(rendering)

//---

void ObjectProxyMeshChunk::updatePassTypes()
{
    if (material)
    {
        const auto& shaderMetadata = material->renderStates();
        if (!shaderMetadata.hasTransparency)
        {
            if (!shaderMetadata.hasVertexAnimation && staticGeometry)
                depthPassType = 0;
            else
                depthPassType = 1;
        }
        else
        {
            depthPassType = -1;
        }

        // forward pass flags
        if (shaderMetadata.hasTransparency)
            forwardPassType = 2;
        else if (shaderMetadata.hasPixelDiscard)
            forwardPassType = 1;
        else
            forwardPassType = 0;
    }
    else
    {
        depthPassType = -1;
        forwardPassType = -1;
    }
}

//---

RTTI_BEGIN_TYPE_NATIVE_CLASS(ObjectProxyMesh);
RTTI_END_TYPE();

ObjectProxyMesh::ObjectProxyMesh()
{}

ObjectProxyMesh::~ObjectProxyMesh()
{}

ObjectProxyMeshPtr ObjectProxyMesh::Compile(const Setup& setup)
{
    DEBUG_CHECK_RETURN_EX_V(setup.mesh, "No source mesh", nullptr);

    const auto maxLods = setup.mesh->detailLevels().size();
    DEBUG_CHECK_RETURN_EX_V(setup.forcedLodLevel == -1 || (setup.forcedLodLevel >= 0 && setup.forcedLodLevel < (int)maxLods), "Invalid LOD index", nullptr);

    InplaceArray<uint16_t, 256> chunksToCreate;
    for (auto i : setup.mesh->chunks().indexRange())
    {
        const auto& chunk = setup.mesh->chunks().typedData()[i];
        const auto& materialName = setup.mesh->materials().typedData()[chunk.materialIndex].name;

        if (!setup.selectiveMaterialMask.empty() && !setup.selectiveMaterialMask.contains(materialName))
            continue;

        if (setup.excludedMaterialMask.contains(materialName))
            continue;

        if (!chunk.proxy) // not renderable
            continue;

        chunksToCreate.pushBack(i);
    }

    if (chunksToCreate.empty())
        return nullptr;

    const auto numLods = (setup.forcedLodLevel == -1) ? maxLods : 1;

    ProxyMemoryAllocator<ObjectProxyMesh> allocator;
    allocator.add<ObjectProxyMeshLOD>(numLods);
    allocator.add<ObjectProxyMeshChunk>(chunksToCreate.size());

    auto* ret = allocator.allocate();
    ret->m_numChunks = 0;// chunksToCreate.size();
    ret->m_numLods = numLods;
    ret->m_localBox = setup.mesh->bounds();

    // setup lod table
    auto* lodTable = (ObjectProxyMeshLOD*)ret->lods();
    if (setup.forcedLodLevel != -1)
    {
		const auto& lastLOD = setup.mesh->detailLevels().back();
		lodTable[0].minDistanceSquared = 0.0f;
        lodTable[0].maxDistanceSquared = lastLOD.rangeMax * lastLOD.rangeMax;
    }
    else
    {
        for (uint32_t i = 0; i < maxLods; ++i)
        {
            const auto& sourceLod = setup.mesh->detailLevels().typedData()[i];
            lodTable[i].minDistanceSquared = sourceLod.rangeMin * sourceLod.rangeMin;
            lodTable[i].maxDistanceSquared = sourceLod.rangeMax * sourceLod.rangeMax;
        }
    }

    // setup chunks
    auto* chunkTable = (ObjectProxyMeshChunk*)ret->chunks();
    for (auto index : chunksToCreate)
    {
        const auto& chunk = setup.mesh->chunks().typedData()[index];

        // determine material to use at chunk
        const IMaterial* sourceMaterial = setup.forceMaterial;
        if (!sourceMaterial)
        {
            // apply override
            const auto& materialName = setup.mesh->materials().typedData()[chunk.materialIndex].name;
            if (const auto* overrideMaterial = setup.materialOverrides.find(materialName))
                if (*overrideMaterial)
                    sourceMaterial = *overrideMaterial;

            // if no override was applied use the mesh's material
            if (!sourceMaterial)
                sourceMaterial = setup.mesh->materials()[chunk.materialIndex].material.get();

            // use the fall back

            // if we still have no material don't render this chunk
            if (!sourceMaterial)
                continue;
        }

        // setup chunk
        chunkTable->data = chunk.proxy;
        chunkTable->materialIndex = index;
        chunkTable->lodMask = chunk.detailMask;
        chunkTable->renderMask = chunk.renderMask;
        chunkTable->material = sourceMaterial->dataProxy();
        chunkTable->shader = sourceMaterial->templateProxy();
        chunkTable->staticGeometry = setup.staticGeometry;
        ASSERT_EX(chunkTable->material != nullptr, "Material without data proxy");
        ASSERT_EX(chunkTable->shader != nullptr, "Material without shader");
        chunkTable->updatePassTypes();

        // write chunk
        chunkTable += 1;
    }

    // count actually written chunks
    ret->m_numChunks = (uint16_t)(chunkTable - ret->chunks());

    return AddRef(ret);
}

float ObjectProxyMesh::visibilityDistanceSquared() const
{
	if (m_numLods)
		return lods()[m_numLods - 1].maxDistanceSquared;
	else
		return 0.0f;
}

uint32_t ObjectProxyMesh::calcDetailMask(float distanceSquared) const
{
	uint32_t mask = 0;
	uint32_t bit = 1;

	const auto* lod = lods();
	const auto* lodEnd = lod + m_numLods;
	while (lod < lodEnd)
	{
		if (distanceSquared >= lod->minDistanceSquared && distanceSquared < lod->maxDistanceSquared)
			mask |= bit;
		bit <<= 1;
		++lod;
	}

	return mask;
}

//---

RTTI_BEGIN_TYPE_CLASS(ObjectManagerMesh);
RTTI_END_TYPE();

ObjectManagerMesh::ObjectManagerMesh()
{}

ObjectManagerMesh::~ObjectManagerMesh()
{}

void ObjectManagerMesh::initialize(Scene* scene, gpu::IDevice* dev)
{
    static auto* materialService = GetService<MaterialService>();
    materialService->registerMaterialProxyChangeListener(this);
}

void ObjectManagerMesh::shutdown()
{
    static auto* materialService = GetService<MaterialService>();
    materialService->unregisterMaterialProxyChangeListener(this);
}

void ObjectManagerMesh::prepare(gpu::CommandWriter& cmd, gpu::IDevice* dev, const FrameRenderer& frame)
{
	m_stats = ObjectMeshTotalStats();
}

void ObjectManagerMesh::finish(gpu::CommandWriter& cmd, gpu::IDevice* dev, const FrameRenderer& frame, FrameStats& outStats)
{
	{
		auto lock = CreateLock(m_statLock);
		m_lastStats = m_stats;
	}

	exportStats(m_lastStats.mainVisibility, outStats.mainView);
	exportStats(m_lastStats.globalShadowsVisibility, outStats.globalShadowView);
    
	exportStats(m_lastStats.depthBatching, outStats.depthView);
	exportStats(m_lastStats.mainBatching, outStats.mainView);
	exportStats(m_lastStats.globalShadowsBatching, outStats.globalShadowView);
    exportStats(m_lastStats.localShadowsBatching, outStats.localShadowView);
}

void ObjectManagerMesh::exportStats(const ObjectMeshBatchingStats& stats, FrameViewStats& outStats) const
{
	outStats.numChunks += stats.numInstances;
	outStats.numDrawCalls += stats.numBatches;
	outStats.numMaterials += stats.numMaterialChanges;
	outStats.numShaders += stats.numShaderChanges;
	outStats.numTriangles += stats.numTriangles;
	outStats.recordingTime += stats.recordingTime;
}

void ObjectManagerMesh::exportStats(const ObjectMeshVisibilityStats& stats, FrameViewStats& outStats) const
{
	outStats.cullingTime += stats.cullingTime;
}

void ObjectManagerMesh::render(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame)
{
	PC_SCOPE_LVL1(RenderMeshesMain);

	auto& collector = m_cacheViewMain;

	collectMainViewChunks(view, collector, m_stats.mainVisibility);

	sortChunksByBatch(collector.depthLists[0].standaloneChunks);
	sortChunksByBatch(collector.depthLists[1].standaloneChunks);

    sortChunksByBatch(collector.forwardLists[0].standaloneChunks);
    sortChunksByBatch(collector.forwardLists[1].standaloneChunks);
	sortChunksByBatch(collector.forwardLists[2].standaloneChunks);

	sortChunksByBatch(collector.selectionOutlineList.standaloneChunks);

	renderChunkListStandalone(cmd.depthPrePassStatic, collector.depthLists[0].standaloneChunks, MaterialPass::DepthPrepass, m_stats.depthBatching);
	renderChunkListStandalone(cmd.depthPrePassOther, collector.depthLists[1].standaloneChunks, MaterialPass::DepthPrepass, m_stats.depthBatching);

    renderChunkListStandalone(cmd.forwardSolid, collector.forwardLists[0].standaloneChunks, MaterialPass::Forward, m_stats.mainBatching);
    renderChunkListStandalone(cmd.forwardMasked, collector.forwardLists[1].standaloneChunks, MaterialPass::Forward, m_stats.mainBatching);
	renderChunkListStandalone(cmd.forwardTransparent, collector.forwardLists[2].standaloneChunks, MaterialPass::ForwardTransparent, m_stats.mainBatching);

	renderChunkListStandalone(cmd.selectionOutline, collector.selectionOutlineList.standaloneChunks, MaterialPass::DepthPrepass, m_stats.depthBatching);
}

void ObjectManagerMesh::render(FrameViewCascadesRecorder& cmd, const FrameViewCascades& view, const FrameRenderer& frame)
{
	PC_SCOPE_LVL1(RenderMeshesCascades);
}

void ObjectManagerMesh::render(FrameViewWireframeRecorder& cmd, const FrameViewWireframe& view, const FrameRenderer& frame)
{
	PC_SCOPE_LVL1(RenderMeshesWireframe);

    auto& collector = m_cacheViewWireframe;

    collectWireframeViewChunks(view, collector);

    sortChunksByBatch(collector.mainList.standaloneChunks);
	sortChunksByBatch(collector.selectionOutlineList.standaloneChunks);

	ObjectMeshBatchingStats stats;

	const auto solid = (frame.frame().mode == FrameRenderMode::WireframeSolid);
	if (solid)
	{
		renderChunkListStandalone(cmd.depthPrePass, collector.mainList.standaloneChunks, MaterialPass::DepthPrepass, stats);
		renderChunkListStandalone(cmd.mainSolid, collector.mainList.standaloneChunks, MaterialPass::WireframeSolid, stats);
	}
	else
	{
		renderChunkListStandalone(cmd.mainSolid, collector.mainList.standaloneChunks, MaterialPass::WireframePassThrough, stats);
	}			

	renderChunkListStandalone(cmd.selectionOutline, collector.selectionOutlineList.standaloneChunks, MaterialPass::DepthPrepass, stats);
}

void ObjectManagerMesh::render(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame)
{
    PC_SCOPE_LVL1(RenderCaptureSelection);

    auto& collector = m_cacheCaptureView;

	collectCaptureChunks(view, collector);

	sortChunksByBatch(collector.mainList.standaloneChunks);

    ObjectMeshBatchingStats stats;
    renderChunkListStandalone(cmd.depthPrePass, collector.mainList.standaloneChunks, MaterialPass::DepthPrepass, stats);
    renderChunkListStandalone(cmd.mainFragments, collector.mainList.standaloneChunks, MaterialPass::SelectionFragments, stats);
}

void ObjectManagerMesh::render(FrameViewCaptureDepthRecorder& cmd, const FrameViewCaptureDepth& view, const FrameRenderer& frame)
{
    PC_SCOPE_LVL1(RenderCaptureSelection);

    auto& collector = m_cacheCaptureView;

	collectCaptureChunks(view, collector);

    sortChunksByBatch(collector.mainList.standaloneChunks);

    ObjectMeshBatchingStats stats;
	renderChunkListStandalone(cmd.depth, collector.mainList.standaloneChunks, MaterialPass::DepthPrepass, stats);
}

//--

ObjectManagerMesh::VisibleChunkList::VisibleChunkList()
{
	standaloneChunks.reserve(1024);
}

void ObjectManagerMesh::VisibleChunkList::prepare(uint32_t totalChunkCount)
{
	if (totalChunkCount > standaloneChunks.capacity())
	{
		const auto roundedCapacity = Align<uint32_t>(totalChunkCount, 1024);
		standaloneChunks.reserve(roundedCapacity);
	}

	standaloneChunks.reset();
	//standaloneChunks.allocateUninitialized(totalChunkCount);
}

void ObjectManagerMesh::VisibleMainViewCollector::prepare(uint32_t totalChunkCount)
{
	selectionOutlineList.prepare(totalChunkCount);

	for (auto& list : depthLists)
		list.prepare(totalChunkCount);

    for (auto& list : forwardLists)
        list.prepare(totalChunkCount);
}

void ObjectManagerMesh::VisibleWireframeViewCollector::prepare(uint32_t totalChunkCount)
{
	selectionOutlineList.prepare(totalChunkCount);
	mainList.prepare(totalChunkCount);
}

void ObjectManagerMesh::VisibleCaptureCollector::prepare(uint32_t totalChunkCount)
{
    mainList.prepare(totalChunkCount);
}

//--

void ObjectManagerMesh::collectMainViewChunks(const FrameViewSingleCamera& view, VisibleMainViewCollector& outCollector, ObjectMeshVisibilityStats& outStats) const
{
	PC_SCOPE_LVL1(CollectMainView);

	ScopeTimer timer;

    // render only chunks that we want to show in the main view
    const auto renderMask = (uint32_t)MeshChunkRenderingMaskBit::Scene;

	// visit all objects
	const auto* objects = m_localObjects.values().typedData();
    const auto* objectsEnd = objects + m_localObjects.values().size();
	outStats.numTestedObjects += m_localObjects.values().size();

    // prepare culling camera
    VisibilityFrustum frustum;
    frustum.setup(view.visibilityCamera());

	// prepare output
	outCollector.prepare(1024);

	// cull objects and collect visible chunks
	while (objects < objectsEnd)
	{
		// cull against frustum
		const auto& object = *objects++;
		if (!object.box.isInFrustum(frustum))
			continue;

		const auto lodDistance = view.lodReferencePoint().squareDistance(object.distanceRefPoint);
		if (lodDistance >= object.maxDistanceSquared)
			continue;

		const auto lodMask = object.data->calcDetailMask(lodDistance);
		if (!lodMask)
			continue;

        const auto selected = object.data->m_flags.test(ObjectProxyFlagBit::Selected);

		outStats.numVisibleObjects += 1;
		outStats.numTestedChunks += object.data->m_numChunks;

		const auto* chunk = object.data->chunks();
		const auto* chunkEnd = chunk + object.data->m_numChunks;
		while (chunk < chunkEnd)
		{
			if (chunk->lodMask & lodMask)
			{
				if (chunk->forwardPassType >= 0)
				{
					outStats.numVisibleChunks += 1;

					auto& visChunk = outCollector.forwardLists[chunk->forwardPassType].standaloneChunks.emplaceBack();
					visChunk.object = object.data;
					visChunk.chunk = (const MeshChunkProxy_Standalone*)chunk->data.get();
					visChunk.material = chunk->material;
					visChunk.shader = chunk->shader;
				}

				if (chunk->depthPassType >= 0)
				{
					outStats.numVisibleChunks += 1;

					auto& visChunk = outCollector.depthLists[chunk->depthPassType].standaloneChunks.emplaceBack();
					visChunk.object = object.data;
					visChunk.chunk = (const MeshChunkProxy_Standalone*)chunk->data.get();
					visChunk.material = chunk->material;
					visChunk.shader = chunk->shader; // TODO: allow fallback to simpler depth-only shader ?
				}

				if (selected && chunk->forwardPassType <= 2) // ignore transparent
				{
					outStats.numVisibleChunks += 1;

					auto& visChunk = outCollector.selectionOutlineList.standaloneChunks.emplaceBack();
					visChunk.object = object.data;
					visChunk.chunk = (const MeshChunkProxy_Standalone*)chunk->data.get();
					visChunk.material = chunk->material;
					visChunk.shader = chunk->shader;
				}
			}

			++chunk;
		}
	}

	outStats.cullingTime += timer.timeElapsed();
}

void ObjectManagerMesh::collectCaptureChunks(const FrameViewSingleCamera& view, VisibleCaptureCollector& outCollector) const
{
	PC_SCOPE_LVL1(CollectMainView);

	// render only chunks that we want to show in the main view
	const auto renderMask = (uint32_t)MeshChunkRenderingMaskBit::Scene;

	// visit all objects
	const auto* objects = m_localObjects.values().typedData();
	const auto* objectsEnd = objects + m_localObjects.values().size();

	// prepare culling camera
	VisibilityFrustum frustum;
	frustum.setup(view.visibilityCamera());

	// prepare output
	outCollector.prepare(1024);

	// cull objects and collect visible chunks
	while (objects < objectsEnd)
	{
		// cull against frustum
		const auto& object = *objects++;
		if (!object.box.isInFrustum(frustum))
			continue;

        const auto lodDistance = view.lodReferencePoint().squareDistance(object.distanceRefPoint);
        if (lodDistance >= object.maxDistanceSquared)
            continue;

        const auto lodMask = object.data->calcDetailMask(lodDistance);
        if (!lodMask)
            continue;

		const auto* chunk = object.data->chunks();
		const auto* chunkEnd = chunk + object.data->m_numChunks;
		while (chunk < chunkEnd)
		{
			if (chunk->lodMask & lodMask)
			{
				auto& visChunk = outCollector.mainList.standaloneChunks.emplaceBack();
				visChunk.object = object.data;
				visChunk.chunk = (const MeshChunkProxy_Standalone*)chunk->data.get();
				visChunk.material = chunk->material;
				visChunk.materialIndex = chunk->materialIndex;
				visChunk.shader = chunk->shader;
			}

			++chunk;
		}
	}
}

void ObjectManagerMesh::collectWireframeViewChunks(const FrameViewSingleCamera& view, VisibleWireframeViewCollector& outCollector) const
{
	PC_SCOPE_LVL1(CollectMainView);

	// render only chunks that we want to show in the main view
	const auto renderMask = (uint32_t)MeshChunkRenderingMaskBit::Scene;

	// visit all objects
	const auto* objects = m_localObjects.values().typedData();
	const auto* objectsEnd = objects + m_localObjects.values().size();

	// prepare culling camera
	VisibilityFrustum frustum;
	frustum.setup(view.visibilityCamera());

	// prepare output
	outCollector.prepare(1024);

	// cull objects and collect visible chunks
	while (objects < objectsEnd)
	{
		// cull against frustum
		const auto& object = *objects++;
		if (!object.box.isInFrustum(frustum))
			continue;

        const auto lodDistance = view.lodReferencePoint().squareDistance(object.distanceRefPoint);
        if (lodDistance >= object.maxDistanceSquared)
            continue;

        const auto lodMask = object.data->calcDetailMask(lodDistance);
        if (!lodMask)
            continue;

		const auto selected = object.data->m_flags.test(ObjectProxyFlagBit::Selected);

		const auto* chunk = object.data->chunks();
		const auto* chunkEnd = chunk + object.data->m_numChunks;
		while (chunk < chunkEnd)
		{
			if (chunk->lodMask & lodMask)
			{
				if (chunk->forwardPassType != 2)
				{
					auto& visChunk = outCollector.mainList.standaloneChunks.emplaceBack();
					visChunk.object = object.data;
					visChunk.chunk = (const MeshChunkProxy_Standalone*)chunk->data.get();
					visChunk.material = chunk->material;
					visChunk.shader = chunk->shader;

					if (selected)
					{
						auto& visChunk = outCollector.selectionOutlineList.standaloneChunks.emplaceBack();
						visChunk.object = object.data;
						visChunk.chunk = (const MeshChunkProxy_Standalone*)chunk->data.get();
						visChunk.material = chunk->material;
						visChunk.shader = chunk->shader;
					}
				}
			}

			++chunk;
		}
	}
}

//--

#pragma pack(push)
#pragma pack(4)
struct GPUInstanceObjectData
{
	Matrix localToWorld;
	Vector4 worldBoundsInfo; // .w = size
    uint32_t selectionObjectID;
	uint32_t selectionSubObjectID;
	Color color;
	Color colorEx;
};

struct GPUInstanceBatchData
{
	static const uint32_t MAX_OBJECTS_IN_BATCH = (1U << 15) / sizeof(GPUInstanceObjectData);
	Vector4 meshQuantizationOffset;
	Vector4 meshQuantizationScale;

	GPUInstanceObjectData objects[MAX_OBJECTS_IN_BATCH];
};
#pragma pack(pop)

void ObjectManagerMesh::sortChunksByBatch(Array<VisibleStandaloneChunk>& chunks) const
{
    std::sort(chunks.begin(), chunks.end(), [](const VisibleStandaloneChunk& a, const VisibleStandaloneChunk& b)
        {
            if (a.shader != b.shader)
                return a.shader < b.shader;
            if (a.material != b.material)
                return a.material < b.material;
            return a.chunk < b.chunk;
        });
}

void ObjectManagerMesh::renderChunkListStandalone(gpu::CommandWriter& cmd, const Array<VisibleStandaloneChunk>& chunks, MaterialPass pass, ObjectMeshBatchingStats& outStats) const
{
	ScopeTimer timer;

    const auto* objects = m_localObjects.values().typedData();

	// last bound data
    const MaterialTemplateProxy* lastBoundShader = nullptr;
    const MaterialDataProxy* lastBoundMaterial = nullptr;
    const MeshChunkProxy_Standalone* lastBoundChunk = nullptr;
	const gpu::GraphicsPipelineObject* lastMaterialPSO = nullptr;
	MeshVertexFormat lastBoundVertexFormat = MeshVertexFormat::MAX;
	uint32_t lastStaticSwitches = 0;

	// render chunks
	const auto* chunk = chunks.typedData();
	const auto* chunkEnd = chunk + chunks.size();
	while (chunk < chunkEnd)
	{
		const auto* chunkLocalEnd = std::min(chunk + GPUInstanceBatchData::MAX_OBJECTS_IN_BATCH, chunkEnd);
        const auto* startChunk = chunk;

		GPUInstanceBatchData batchData;
		batchData.meshQuantizationOffset = startChunk->chunk->quantizationOffset();
		batchData.meshQuantizationScale = startChunk->chunk->quantizationScale();

		auto* batchObject = batchData.objects;

		while (chunk < chunkLocalEnd)
		{
			if (chunk->material != startChunk->material)
				break;
            if (chunk->shader != startChunk->shader)
                break;
            if (chunk->chunk != startChunk->chunk)
                break;

			batchObject->localToWorld = chunk->object->m_localToWorld.transposed();
            batchObject->selectionObjectID = chunk->object->m_selectable.objectID();
            batchObject->selectionSubObjectID = chunk->object->m_selectable.subObjectID() ? chunk->object->m_selectable.subObjectID() : chunk->materialIndex;
			batchObject->worldBoundsInfo = chunk->object->m_localToWorld.translation();
			batchObject->color = chunk->object->m_color;
			batchObject->colorEx = chunk->object->m_colorEx;
			++batchObject;
			++chunk;
		}

		// bind chunk vertex&index buffer
		if (lastBoundChunk != startChunk->chunk)
		{
			startChunk->chunk->bind(cmd);
			lastBoundChunk = startChunk->chunk;
			outStats.numGeometryChanges += 1;

			if (lastBoundVertexFormat != lastBoundChunk->format())
			{
				lastBoundVertexFormat = lastBoundChunk->format();
				lastBoundShader = nullptr; // rebind material on vertex format change
			}					
		}

		// bind material parameters
		bool forceRebindMaterial = false;
		if (lastBoundMaterial != startChunk->material)
		{
			uint32_t staticSwitchMask = 0;

			outStats.numMaterialChanges += 1;
			startChunk->material->bind(cmd, staticSwitchMask);
			lastBoundMaterial = startChunk->material;

			if (lastStaticSwitches != staticSwitchMask)
			{
				outStats.numShaderVariantChanges += 1;
				lastStaticSwitches = staticSwitchMask;
				forceRebindMaterial = true;
			}
		}

		// select material technique
		if (lastBoundShader != startChunk->shader || forceRebindMaterial)
		{
			MaterialCompilationSetup setup;
			setup.bindlessTextures = false;
			setup.meshletsVertices = false;
			setup.vertexFormat = lastBoundVertexFormat;
			setup.staticSwitches = lastStaticSwitches;
			setup.pass = pass;
			setup.msaa = false;

			if (auto technique = startChunk->shader->fetchTechnique(setup))
			{
				auto* pso = technique->pso();
				if (pso != lastMaterialPSO)
				{
					outStats.numShaderChanges += 1;
					lastMaterialPSO = pso;
				}
			}

			lastBoundShader = startChunk->shader;
		}

		// bind instance data for this batch
		{
			gpu::DescriptorEntry desc[1];
			desc[0].constants(&batchData, (char*)batchObject - (char*)&batchData); // upload only used data
			cmd.opBindDescriptor("InstanceDataDesc"_id, desc);
		}

		// draw!
		if (lastMaterialPSO)
		{
			const auto numInstances = (uint32_t)(chunk - startChunk);
			lastBoundChunk->draw(lastMaterialPSO, cmd, numInstances);
			outStats.numBatches += 1;
			outStats.numInstances += numInstances;
			outStats.numTriangles += lastBoundChunk->indexCount() / 3;
		}
	}

	outStats.numExecutions += 1;
	outStats.recordingTime += timer.timeElapsed();
}

void ObjectManagerMesh::handleMaterialProxyChanges(const MaterialDataProxyChangesRegistry& changedProxies)
{
	for (const auto& object : m_localObjects.values())
	{
		auto* chunk = object.data->chunks();
		auto* chunkEnd = chunk + object.data->m_numChunks;
		while (chunk < chunkEnd)
		{
			if (PatchMaterialProxy(chunk->material, changedProxies))
			{
				chunk->shader = AddRef(chunk->material->templateProxy());
				chunk->updatePassTypes();
			}

			++chunk;
		}
	}
}

//--

void ObjectManagerMesh::attachProxy(ObjectProxyMeshPtr meshProxy)
{
	runNowOrBuffer([this, meshProxy]() {
		DEBUG_CHECK_RETURN_EX(meshProxy, "No mesh proxy");
		DEBUG_CHECK_RETURN_EX(!m_localObjects.contains(meshProxy), "Proxy already registered");

		const auto box = meshProxy->m_localToWorld.transformBox(meshProxy->m_localBox);

		LocalObject obj;
		obj.box.setup(box);
		obj.data = meshProxy;
		obj.distanceRefPoint = box.center();
		obj.maxDistanceSquared = meshProxy->visibilityDistanceSquared();

		m_localObjects[meshProxy] = obj;
		});
}

void ObjectManagerMesh::detachProxy(ObjectProxyMeshPtr meshProxy)
{
	runNowOrBuffer([this, meshProxy]() {
		DEBUG_CHECK_RETURN_EX(m_localObjects.contains(meshProxy), "Proxy not registered");
		m_localObjects.remove(meshProxy);
		});
}

void ObjectManagerMesh::moveProxy(ObjectProxyMeshPtr meshProxy, Matrix localToWorld)
{
	runNowOrBuffer([this, meshProxy, localToWorld]() {
		meshProxy->m_localToWorld = localToWorld;

		auto* localObject = m_localObjects.find(meshProxy);
		DEBUG_CHECK_RETURN_EX(localObject != nullptr, "Proxy not registered");

		const auto* proxy = localObject->data.get();
		const auto box = proxy->m_localToWorld.transformBox(proxy->m_localBox);
		localObject->box.setup(box);
		localObject->distanceRefPoint = box.center();
		});
}

void ObjectManagerMesh::updateProxyFlag(ObjectProxyMeshPtr meshProxy, ObjectProxyFlags clearFlags, ObjectProxyFlags setFlags)
{
	runNowOrBuffer([this, meshProxy, clearFlags, setFlags]() {
		auto* localObject = m_localObjects.find(meshProxy);
		DEBUG_CHECK_RETURN_EX(localObject != nullptr, "Proxy not registered");

		auto* proxy = localObject->data.get();
		proxy->m_flags -= clearFlags;
		proxy->m_flags |= setFlags;
		});
}

//--

END_BOOMER_NAMESPACE_EX(rendering)
