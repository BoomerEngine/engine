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

#include "rendering/mesh/include/renderingMeshService.h"
#include "rendering/material/include/renderingMaterialRuntimeTechnique.h"
#include "rendering/material/include/renderingMaterialRuntimeLayout.h"
#include "rendering/material/include/renderingMaterialRuntimeProxy.h"
#include "rendering/material/include/renderingMaterialTemplate.h"

namespace rendering
{
    namespace scene
    {
        //--

#pragma pack(push)
#pragma pack(4)
        struct GPUChunkInfo
        {
            ObjectRenderID objectId; // object - gives the transform
            uint32_t meshChunkId; // index in the global geometry table
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
        }

        void FragmentHandler_Mesh::handleSceneLock()
        {}

        void FragmentHandler_Mesh::handleSceneUnlock()
        {}

        void FragmentHandler_Mesh::handlePrepare(command::CommandWriter& cmd)
        {}

        void FragmentHandler_Mesh::handleRender(command::CommandWriter& cmd, const FrameView& view, const FragmentRenderContext& context, const Fragment* const* fragments, uint32_t numFragments) const
        {
            TransientBufferView bufferChunkData(BufferViewFlag::ShaderReadable, TransientBufferAccess::ShaderReadOnly, sizeof(GPUChunkInfo) * numFragments, sizeof(GPUChunkInfo));

            // skip meshes if disabled
            if (!(view.frame().filters & FilterBit::Meshes))
                return;

            // prepare data buffer
            {
                auto* renderChunkData = cmd.opAllocTransientBufferWithUninitializedTypedData<GPUChunkInfo>(bufferChunkData, numFragments);
                for (FragmentIterator<Fragment_Mesh> it(fragments, numFragments); it; ++it)
                {
                    renderChunkData->objectId = it->objectId;
                    renderChunkData->meshChunkId = it->meshChunkdId;
                    renderChunkData += 1;
                }
            }

            // bind the data
            GPUMeshDrawData params;
            params.drawChunks = bufferChunkData;
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

                const auto drawCount = it.pos() - drawFirstIndex;
                const auto renderChunkIndexCount = m_meshCache->chunkInfo(renderMeshChunkId).indexCount;
                const auto renderChunkVertexCount = m_meshCache->chunkInfo(renderMeshChunkId).vertexCount;

                //cmd.opSetFillState(PolygonMode::Line);

                const auto& state = renderMaterialTemplate->fetchTechnique(context.pass)->state();
                if (state.shader)
                {
                    if (context.allowsCustomRenderStates)
                    {
                        cmd.opSetCullState(state.renderStates.twoSided ? CullMode::Disabled : CullMode::Back);
                        cmd.opSetDepthState(state.renderStates.depthTest, state.renderStates.depthWrite);

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
                    }
                }

                //++it;
            }

            // restore states
            if (context.allowsCustomRenderStates)
            {
                cmd.opSetCullState(CullMode::Back);
                cmd.opSetDepthState(true, true);
                cmd.opSetMultisampleState(MultisampleState());
            }
        }

        //--

    } // scene
} // rendering
