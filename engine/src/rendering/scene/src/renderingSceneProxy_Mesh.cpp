/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\proxy #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneCulling.h"
#include "renderingSceneProxyDesc.h"
#include "renderingSceneProxy_Mesh.h"
#include "renderingSceneFragment_Mesh.h"
#include "renderingSceneFragmentList.h"
#include "renderingMaterialCache.h"

#include "rendering/mesh/include/renderingMesh.h"
#include "rendering/mesh/include/renderingMeshService.h"
#include "rendering/material/include/renderingMaterial.h"
#include "rendering/material/include/renderingMaterialInstance.h"
#include "rendering/material/include/renderingMaterialTemplate.h"
#include "rendering/material/include/renderingMaterialRuntimeProxy.h"

namespace rendering
{
    namespace scene
    {

        //--

        ProxyMesh::ProxyMesh()
            : IProxy(ProxyType::Mesh)
        {}

        //--

        RTTI_BEGIN_TYPE_CLASS(ProxyMeshHandler);
        RTTI_END_TYPE();

        ProxyMeshHandler::ProxyMeshHandler()
            : IProxyHandler(ProxyType::Mesh)
            , m_pool(1024)
        {
            m_materialCache = base::GetService<MaterialCache>();

            base::GetService<MaterialService>()->registerMaterialProxyChangeListener(this);
        }

        ProxyMeshHandler::~ProxyMeshHandler()
        {
            base::GetService<MaterialService>()->unregisterMaterialProxyChangeListener(this);
        }

        void ProxyMeshHandler::handleInit(Scene* scene)
        {
            TBaseClass::handleInit(scene);
        }

        void ProxyMeshHandler::handleSceneLock()
        {
            TBaseClass::handleSceneLock();
        }

        void ProxyMeshHandler::handleSceneUnlock()
        {
            TBaseClass::handleSceneUnlock();
        }

        void ProxyMeshHandler::handlePrepare(command::CommandWriter& cmd)
        {
            TBaseClass::handlePrepare(cmd);
        }

        static base::res::StaticResource<IMaterial> resFallbackMaterial("engine/materials/fallback.v4mg");

        static MaterialDataProxyPtr ResolveMaterial(base::StringID name, IMaterial* material, const ProxyMeshDesc& settings)
        {
            if (settings.forceMaterial)
                return settings.forceMaterial;

            if (const auto overrideMaterialPtr = settings.materialOverrides.find(name))
            {
                if (*overrideMaterialPtr)
                {
                    if (auto materialData = (*overrideMaterialPtr)->dataProxy())
                        return materialData;
                }
            }

            if (material)
                if (auto materialData = material->dataProxy())
                    return materialData;

            if (auto fallbackMaterial = resFallbackMaterial.loadAndGet())
                if (auto materialData = fallbackMaterial->dataProxy())
                    return materialData;

            return nullptr;
        }

        IProxy* ProxyMeshHandler::handleProxyCreate(const ProxyBaseDesc& desc)
        {
            const auto& meshDesc = desc.cast<ProxyMeshDesc>();

            // no mesh specified
            if (!meshDesc.mesh)
                return nullptr;

            // mesh has no chunks
            if (meshDesc.mesh->chunks().empty())
                return nullptr;

            // create proxy 
            auto* proxy = new (m_pool.alloc()) ProxyMesh();

            // allocate object
            SceneObjectInfo objectInfo;
            objectInfo.autoHideDistance = (meshDesc.autoHideDistanceOverride > 0.0f) ? meshDesc.autoHideDistanceOverride : 100.0f; // TODO
            objectInfo.localToScene = meshDesc.localToScene;
            objectInfo.proxyPtr = proxy;
            objectInfo.proxyType = ProxyType::Mesh;
            objectInfo.sceneBounds = meshDesc.localToScene.transformBox(meshDesc.mesh->bounds().box); // initial bounds
            if (!scene()->objects().registerObject(objectInfo, proxy->objectId))
            {
                proxy->~ProxyMesh();
                m_pool.release(proxy);
                return nullptr;
            }

            // TODO: create LOD ranges

            // create chunk fragments
            proxy->localBounds = meshDesc.mesh->bounds().box;
            proxy->chunks.reserve(meshDesc.mesh->chunks().size());
            for (const auto& meshChunk : meshDesc.mesh->chunks())
            {
                const auto& chunkMaterial = meshDesc.mesh->materials()[meshChunk.materialIndex];
                const auto materialData = ResolveMaterial(chunkMaterial.name, chunkMaterial.material, meshDesc);
                if (!materialData)
                    continue; // skip over chunks that do not have any valid material

                const auto& materialTemplate = materialData->materialTemplate();
                const auto materialCachedTemplate = m_materialCache->mapTemplate(materialTemplate.lock(), meshChunk.vertexFormat);
                if (!materialCachedTemplate)
                    continue; // skip chunks that have invalid material template

                // create the chunk data
                auto& chunk = proxy->chunks.emplaceBack();
                chunk.meshChunkId = meshChunk.renderId;
                chunk.materialTemplate = materialCachedTemplate;
                chunk.materialData = materialData;
            }

            // add to list of all proxies
            m_allProxies.insert(proxy);

            // done
            return proxy;
        }

        void ProxyMeshHandler::handleProxyDestroy(IProxy* proxy)
        {
            auto* localProxy = static_cast<ProxyMesh*>(proxy);

            // remove from list of all proxies
            m_allProxies.remove(localProxy);

            // remove object
            scene()->objects().unregisterObject(localProxy->objectId);

            // TODO: free any data here

            // release to pool
            localProxy->~ProxyMesh();
            m_pool.release(localProxy);
        }

        void ProxyMeshHandler::handleProxyFragments(command::CommandWriter& cmd, FrameView& view, const SceneObjectCullingEntry* proxies, uint32_t numProxies, FragmentDrawList& outFragmentList) const
        {
            for (ProxyIterator<ProxyMesh> it(proxies, numProxies); it; ++it)
            {
                for (const auto& chunk : it->chunks)
                {
                    auto* frag = outFragmentList.allocFragment<Fragment_Mesh>();
                    frag->objectId = it->objectId;
                    frag->materialTemplate = chunk.materialTemplate;
                    frag->meshChunkdId = chunk.meshChunkId;
                    frag->materialData = chunk.materialData;

                    outFragmentList.collectFragment(frag, FragmentDrawBucket::OpaqueNotMoving);
                }
            }
        }

        void ProxyMeshHandler::runMoveProxy(Scene* scene, IProxy* proxy, const CommandMoveProxy& cmd)
        {
            auto* localProxy = static_cast<ProxyMesh*>(proxy);

            if (localProxy->objectId)
            {
                const auto newBounds = cmd.localToScene.transformBox(localProxy->localBounds);
                scene->objects().updateObject(localProxy->objectId, cmd.localToScene, newBounds);
            }
        }

        void ProxyMeshHandler::handleMaterialProxyChanges(const MaterialDataProxyChangesRegistry& changedProxies)
        {
            for (auto* proxy : m_allProxies.keys())
            {
                for (auto& chunk : proxy->chunks)
                {
                    if (PatchMaterialProxy(chunk.materialData, changedProxies))
                    {
                        const auto vertexFormat = chunk.materialTemplate->vertexFormat();
                        const auto& materialTemplate = chunk.materialData->materialTemplate();
                        chunk.materialTemplate = m_materialCache->mapTemplate(materialTemplate.lock(), vertexFormat);
                    }
                }
            }
        }

        //--

    } // scene
} // rendering
