/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#include "build.h"
#include "renderingSceneManager_Mesh.h"
#include "renderingFrameView_Main.h"
#include "renderingFrameView_Cascades.h"
#include "renderingSceneObjects.h"

#include "rendering/mesh/include/renderingMeshChunkProxy.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/material/include/renderingMaterialRuntimeProxy.h"
#include "rendering/material/include/renderingMaterialRuntimeTemplate.h"
#include "rendering/material/include/renderingMaterialRuntimeTechnique.h"
#include "renderingFrameView_Wireframe.h"
#include "renderingFrameView_CaptureSelection.h"

namespace rendering
{
    namespace scene
    {

        //---

		RTTI_BEGIN_TYPE_CLASS(ObjectManagerMesh);
		RTTI_END_TYPE();

		ObjectManagerMesh::ObjectManagerMesh()
		{}

		ObjectManagerMesh::~ObjectManagerMesh()
		{}

		void ObjectManagerMesh::initialize(Scene* scene, IDevice* dev)
		{			
		}

		void ObjectManagerMesh::shutdown()
		{
		}

		void ObjectManagerMesh::prepare(command::CommandWriter& cmd, IDevice* dev, const FrameRenderer& frame)
		{

		}

		void ObjectManagerMesh::render(FrameViewMainRecorder& cmd, const FrameViewMain& view, const FrameRenderer& frame)
		{
			PC_SCOPE_LVL1(RenderMeshesMain);

			auto& collector = m_cacheViewMain;

			collectMainViewChunks(view, collector);

			sortChunksByBatch(collector.depthLists[0].standaloneChunks);
			sortChunksByBatch(collector.depthLists[1].standaloneChunks);

            sortChunksByBatch(collector.forwardLists[0].standaloneChunks);
            sortChunksByBatch(collector.forwardLists[1].standaloneChunks);
			sortChunksByBatch(collector.forwardLists[2].standaloneChunks);

			sortChunksByBatch(collector.selectionOutlineList.standaloneChunks);

			renderChunkListStandalone(cmd.depthPrePassStatic, collector.depthLists[0].standaloneChunks, MaterialPass::DepthPrepass);
			renderChunkListStandalone(cmd.depthPrePassOther, collector.depthLists[1].standaloneChunks, MaterialPass::DepthPrepass);

            renderChunkListStandalone(cmd.forwardSolid, collector.forwardLists[0].standaloneChunks, MaterialPass::Forward);
            renderChunkListStandalone(cmd.forwardMasked, collector.forwardLists[1].standaloneChunks, MaterialPass::Forward);
			renderChunkListStandalone(cmd.forwardTransparent, collector.forwardLists[2].standaloneChunks, MaterialPass::ForwardTransparent);

			renderChunkListStandalone(cmd.selectionOutline, collector.selectionOutlineList.standaloneChunks, MaterialPass::DepthPrepass);
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

			const auto solid = (frame.frame().mode == FrameRenderMode::WireframeSolid);
			if (solid)
			{
				renderChunkListStandalone(cmd.depthPrePass, collector.mainList.standaloneChunks, MaterialPass::DepthPrepass);
				renderChunkListStandalone(cmd.mainSolid, collector.mainList.standaloneChunks, MaterialPass::WireframeSolid);
			}
			else
			{
				renderChunkListStandalone(cmd.mainSolid, collector.mainList.standaloneChunks, MaterialPass::WireframePassThrough);
			}			

			renderChunkListStandalone(cmd.selectionOutline, collector.selectionOutlineList.standaloneChunks, MaterialPass::DepthPrepass);
		}

        void ObjectManagerMesh::render(FrameViewCaptureSelectionRecorder& cmd, const FrameViewCaptureSelection& view, const FrameRenderer& frame)
        {
            PC_SCOPE_LVL1(RenderCaptureSelection);

            auto& collector = m_cacheCaptureView;

            collectSelectionChunks(view, collector);

			sortChunksByBatch(collector.mainList.standaloneChunks);

            renderChunkListStandalone(cmd.depthPrePass, collector.mainList.standaloneChunks, MaterialPass::DepthPrepass);
            renderChunkListStandalone(cmd.mainFragments, collector.mainList.standaloneChunks, MaterialPass::SelectionFragments);
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
				const auto roundedCapacity = base::Align<uint32_t>(totalChunkCount, 1024);
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

		void ObjectManagerMesh::collectMainViewChunks(const FrameViewMain& view, VisibleMainViewCollector& outCollector) const
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

				const auto lodMask = 1; // TODO: compute lod mask

                const auto selected = object.data->m_flags.test(ObjectProxyFlagBit::Selected);

				const auto* chunk = object.data->chunks();
				const auto* chunkEnd = chunk + object.data->m_numChunks;
				while (chunk < chunkEnd)
				{
					if (!(chunk->lodMask & lodMask))
						continue;

					/*if (!(chunk->renderMask & renderMask))
						continue;*/

					if (chunk->forwardPassType >= 0)
					{
						auto& visChunk = outCollector.forwardLists[chunk->forwardPassType].standaloneChunks.emplaceBack();
						visChunk.object = object.data;
						visChunk.chunk = (const MeshChunkProxy_Standalone*)chunk->data.get();
						visChunk.material = chunk->material;
						visChunk.shader = chunk->shader;
					}

                    if (chunk->depthPassType >= 0)
                    {
                        auto& visChunk = outCollector.depthLists[chunk->depthPassType].standaloneChunks.emplaceBack();
                        visChunk.object = object.data;
                        visChunk.chunk = (const MeshChunkProxy_Standalone*)chunk->data.get();
                        visChunk.material = chunk->material;
                        visChunk.shader = chunk->shader; // TODO: allow fallback to simpler depth-only shader ?
                    }

					if (selected && chunk->forwardPassType <= 2) // ignore transparent
					{
						auto& visChunk = outCollector.selectionOutlineList.standaloneChunks.emplaceBack();
                        visChunk.object = object.data;
                        visChunk.chunk = (const MeshChunkProxy_Standalone*)chunk->data.get();
                        visChunk.material = chunk->material;
                        visChunk.shader = chunk->shader;
					}

					++chunk;
				}
			}
		}

		void ObjectManagerMesh::collectSelectionChunks(const FrameViewCaptureSelection& view, VisibleCaptureCollector& outCollector) const
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

				const auto lodMask = 1; // TODO: compute lod mask

				const auto* chunk = object.data->chunks();
				const auto* chunkEnd = chunk + object.data->m_numChunks;
				while (chunk < chunkEnd)
				{
					if (!(chunk->lodMask & lodMask))
						continue;

					/*if (!(chunk->renderMask & renderMask))
						continue;*/

					//if (chunk->forwardPassType != 2)
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

		void ObjectManagerMesh::collectWireframeViewChunks(const FrameViewWireframe& view, VisibleWireframeViewCollector& outCollector) const
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

				const auto lodMask = 1; // TODO: compute lod mask

				const auto selected = object.data->m_flags.test(ObjectProxyFlagBit::Selected);

				const auto* chunk = object.data->chunks();
				const auto* chunkEnd = chunk + object.data->m_numChunks;
				while (chunk < chunkEnd)
				{
					if (!(chunk->lodMask & lodMask))
						continue;

					/*if (!(chunk->renderMask & renderMask))
						continue;*/

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

					++chunk;
				}
			}
		}

		//--

#pragma pack(push)
#pragma pack(4)
		struct GPUInstanceObjectData
		{
			base::Matrix localToWorld;
			base::Vector4 worldBoundsInfo; // .w = size
            uint32_t selectionObjectID;
			uint32_t selectionSubObjectID;
			base::Color color;
			base::Color colorEx;
		};

		struct GPUInstanceBatchData
		{
			static const uint32_t MAX_OBJECTS_IN_BATCH = (1U << 15) / sizeof(GPUInstanceObjectData);
			base::Vector4 meshQuantizationOffset;
			base::Vector4 meshQuantizationScale;

			GPUInstanceObjectData objects[MAX_OBJECTS_IN_BATCH];
		};
#pragma pack(pop)

		void ObjectManagerMesh::sortChunksByBatch(base::Array<VisibleStandaloneChunk>& chunks) const
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

        void ObjectManagerMesh::renderChunkListStandalone(command::CommandWriter& cmd, const base::Array<VisibleStandaloneChunk>& chunks, MaterialPass pass) const
        {
            const auto* objects = m_localObjects.values().typedData();

			// last bound data
            const MaterialTemplateProxy* lastBoundShader = nullptr;
            const MaterialDataProxy* lastBoundMaterial = nullptr;
            const MeshChunkProxy_Standalone* lastBoundChunk = nullptr;
			const GraphicsPipelineObject* lastMaterialPSO = nullptr;
			MeshVertexFormat lastBoundVertexFormat = MeshVertexFormat::MAX;

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

					if (lastBoundVertexFormat != lastBoundChunk->format())
					{
						lastBoundVertexFormat = lastBoundChunk->format();
						lastBoundShader = nullptr; // rebind material on vertex format change
					}					
				}

				// bind material parameters
				if (lastBoundMaterial != startChunk->material)
				{
					startChunk->material->bind(cmd);
					lastBoundMaterial = startChunk->material;
				}

				// select material technique
				if (lastBoundShader != startChunk->shader)
				{
					MaterialCompilationSetup setup;
					setup.bindlessTextures = false;
					setup.meshletsVertices = false;
					setup.vertexFormat = lastBoundVertexFormat;
					setup.pass = pass;
					setup.msaa = false;

					if (auto technique = startChunk->shader->fetchTechnique(setup))
						lastMaterialPSO = technique->pso();

					lastBoundShader = startChunk->shader;
				}

				// bind instance data for this batch
				{
					DescriptorEntry desc[1];
					desc[0].constants(&batchData, (char*)batchObject - (char*)&batchData); // upload only used data
					cmd.opBindDescriptor("InstanceDataDesc"_id, desc);
				}

				// draw!
				if (lastMaterialPSO)
				{
					const auto numInstances = (uint32_t)(chunk - startChunk);
					lastBoundChunk->draw(lastMaterialPSO, cmd, numInstances);
				}
			}
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

		void ObjectManagerMesh::run(const CommandAttachObject& op)
		{
			auto meshProxy = base::rtti_cast<ObjectProxyMesh>(op.proxy);
			DEBUG_CHECK_RETURN_EX(meshProxy, "No mesh proxy");
			DEBUG_CHECK_RETURN_EX(!m_localObjects.contains(meshProxy), "Proxy already registered");

			LocalObject obj;
			obj.box.setup(meshProxy->m_localToWorld.transformBox(meshProxy->m_localBox));
			obj.data = meshProxy;

			m_localObjects[meshProxy] = obj;
		}

		void ObjectManagerMesh::run(const CommandDetachObject& op)
		{
			auto meshProxy = base::rtti_cast<ObjectProxyMesh>(op.proxy);

            DEBUG_CHECK_RETURN_EX(m_localObjects.contains(meshProxy), "Proxy not registered");
			m_localObjects.remove(meshProxy);
		}

		void ObjectManagerMesh::run(const CommandMoveObject& op)
		{
			auto meshProxy = base::rtti_cast<ObjectProxyMesh>(op.proxy);

			meshProxy->m_localToWorld = op.localToWorld;

			auto* localObject = m_localObjects.find(meshProxy);
			DEBUG_CHECK_RETURN_EX(localObject != nullptr, "Proxy not registered");

			const auto* proxy = localObject->data.get();
			localObject->box.setup(proxy->m_localToWorld.transformBox(proxy->m_localBox));
		}

		void ObjectManagerMesh::run(const CommandChangeFlags& op)
		{
            auto meshProxy = base::rtti_cast<ObjectProxyMesh>(op.proxy);

            auto* localObject = m_localObjects.find(meshProxy);
            DEBUG_CHECK_RETURN_EX(localObject != nullptr, "Proxy not registered");

			auto* proxy = localObject->data.get();
			proxy->m_flags -= op.clearFlags;
			proxy->m_flags |= op.setFlags;
		}

        //---

    } // scene
} // rendering