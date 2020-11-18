/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene\fragments #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingSceneFragment_Mesh.h"
#include "renderingMaterialCache.h"
#include "renderingFrameFilters.h"
#include "renderingFrameView.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/mesh/include/renderingMeshService.h"
#include "rendering/material/include/renderingMaterialRuntimeTechnique.h"
#include "rendering/material/include/renderingMaterialRuntimeLayout.h"
#include "rendering/material/include/renderingMaterialRuntimeProxy.h"
#include "rendering/material/include/renderingMaterialTemplate.h"
#include "../../device/include/renderingDeviceService.h"

namespace rendering
{
    namespace scene
    {

        //--

        base::ConfigProperty<uint32_t> cvMeshHandlerChunksPerPass("Rendering.Mesh", "MeshHandlerChunksPerPass", 1024);

        //--

#pragma pack(push)
#pragma pack(4)
        struct GPUChunkInfo
        {
            ObjectRenderID objectId; // object - gives the transform
            uint32_t meshChunkId; // index in the global geometry table
            uint32_t subObjectID;
        };

        struct GPUMeshDrawData
        {
            BufferView drawChunks;
        };
#pragma pack(pop)

        //--

        RTTI_BEGIN_TYPE_CLASS(FragmentHandler_Mesh);
        RTTI_END_TYPE();

        FragmentHandler_Mesh::FragmentHandler_Mesh()
            : IFragmentHandler(FragmentHandlerType::Mesh)
        {}

        FragmentHandler_Mesh::~FragmentHandler_Mesh()
        {}

        void FragmentHandler_Mesh::handleInit(Scene* scene)
        {
            m_meshCache = base::GetService<MeshService>();
            m_bufferChunkDataCapacity = std::max<uint32_t>(cvMeshHandlerChunksPerPass.get(), 16);

            {
                BufferCreationInfo info;
                info.allowShaderReads = true;
                info.allowDynamicUpdate = true;
                info.allowUAV = true;
                info.label = "MeshFrameHandler_ChunkInfo";
                info.size = m_bufferChunkDataCapacity * sizeof(GPUChunkInfo);
                info.stride = sizeof(GPUChunkInfo);

                auto dev = base::GetService<DeviceService>()->device();
                m_bufferChunkData = dev->createBuffer(info);
            }
        }

        void FragmentHandler_Mesh::handleSceneLock()
        {}

        void FragmentHandler_Mesh::handleSceneUnlock()
        {}

        void FragmentHandler_Mesh::handlePrepare(command::CommandWriter& cmd)
        {}

        void FragmentHandler_Mesh::handleRender(command::CommandWriter& cmd, const FrameView& view, const FragmentRenderContext& context, const Fragment* const* fragments, uint32_t numFragments, FrameFragmentRenderStats& outStats) const
        {
            // skip meshes if disabled
            if (!(view.frame().filters & FilterBit::Meshes))
                return;

            // split in parts
            while (numFragments > 0)
            {
                auto numBatchFragments = std::min<uint32_t>(numFragments, m_bufferChunkDataCapacity);
                innerRender(cmd, view, context, fragments, numBatchFragments, outStats);

                numFragments -= numBatchFragments;
                fragments += numBatchFragments;
            }
        }

        void FragmentHandler_Mesh::innerRender(command::CommandWriter& cmd, const FrameView& view, const FragmentRenderContext& context, const Fragment* const* fragments, uint32_t numFragments, FrameFragmentRenderStats& outStats) const
        {
            DEBUG_CHECK(numFragments < m_bufferChunkDataCapacity);

            // count calls to handler
            outStats.numBursts += 1;

            // sort mesh fragments
            std::sort((const Fragment**)fragments, (const Fragment**)fragments + numFragments, [](const Fragment* a, const Fragment* b)
                {
                    const auto* am = static_cast<const Fragment_Mesh*>(a);
                    const auto* bm = static_cast<const Fragment_Mesh*>(b);
                    if (am->materialTemplate != bm->materialTemplate)
                        return am->materialTemplate < bm->materialTemplate;
                    if (am->meshChunkdId != bm->meshChunkdId)
                        return am->meshChunkdId < bm->meshChunkdId;
                    if (am->materialData != bm->materialData)
                        return am->materialData < bm->materialData;
                    return am->objectId < bm->objectId;
                });

            // prepare data buffer
            {
                auto* renderChunkData = cmd.opUpdateDynamicBufferPtrN<GPUChunkInfo>(m_bufferChunkData->view(), 0, numFragments);
                for (FragmentIterator<Fragment_Mesh> it(fragments, numFragments); it; ++it)
                {
                    renderChunkData->objectId = it->objectId;
                    renderChunkData->meshChunkId = it->meshChunkdId;
                    renderChunkData->subObjectID = it->subObjectID;
                    renderChunkData += 1;
                }
            }

            // bind the data
            GPUMeshDrawData params;
            params.drawChunks = m_bufferChunkData->view();
            cmd.opBindParametersInline("MeshDrawData"_id, params);

            // draw all fragments
            // TODO: batching etc
            FragmentIterator<Fragment_Mesh> it(fragments, numFragments);
            while (it)
            {
                auto renderMeshChunkId = it->meshChunkdId;
                auto renderMaterialTemplate = it->materialTemplate;
                auto renderData = it->materialData;

                const auto drawFirstIndex = it.pos();
                while (const auto* next = it.advance())
                    if (next->meshChunkdId != renderMeshChunkId || next->materialTemplate != renderMaterialTemplate || next->materialData != renderData)
                        break;

                if (renderMeshChunkId == 0)
                    continue;

                const auto drawCount = it.pos() - drawFirstIndex;
                const auto renderChunkIndexCount = m_meshCache->chunkInfo(renderMeshChunkId).indexCount;
                const auto renderChunkVertexCount = m_meshCache->chunkInfo(renderMeshChunkId).vertexCount;

                outStats.numFragments += drawCount;
                outStats.numDrawBaches += 1;

                //cmd.opSetFillState(PolygonMode::Line);

                const auto& state = renderMaterialTemplate->fetchTechnique(context.pass)->state();
                if (state.shader)
                {
                    cmd.opSetCullState(state.renderStates.twoSided ? CullMode::Disabled : CullMode::Back);

                    if (context.allowsCustomRenderStates)
                    {
                        cmd.opSetDepthState(state.renderStates.depthTest, state.renderStates.depthWrite, context.depthCompare);

                        if (context.msaaCount > 1)
                        {
                            MultisampleState ms;
                            ms.sampleCount = context.msaaCount;
                            ms.alphaToCoverageEnable = state.renderStates.alphaToCoverage;
                            ms.alphaToOneEnable = true;
                            ms.alphaToCoverageDitherEnable = true;
                            cmd.opSetMultisampleState(ms);
                        }
                    }

                    if (renderData->layout() == state.dataLayout)
                    {
                        cmd.opBindParameters(state.dataLayout->descriptorName(), renderData->upload(cmd));
                        cmd.opDrawInstanced(state.shader, 0, renderChunkIndexCount, drawFirstIndex, drawCount);
                        outStats.numDrawTriangles += (renderChunkIndexCount / 3) * drawCount;
                    }
                }

                //++it;
            }

            // restore states
            if (context.allowsCustomRenderStates)
            {
                cmd.opSetDepthState(true, true);
                cmd.opSetMultisampleState(MultisampleState());
            }

            // reset cull
            cmd.opSetCullState(CullMode::Back);
        }

        //--

    } // scene
} // rendering
