/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#include "build.h"
#include "scene.h"
#include "renderer.h"
#include "cameraContext.h"
#include "params.h"
#include "debugGeometryAssets.h"

#include "helperDebug.h"

#include "gpu/device/include/commandWriter.h"
#include "gpu/device/include/commandBuffer.h"
#include "gpu/device/include/device.h"
#include "gpu/device/include/deviceService.h"
#include "gpu/device/include/shaderData.h"
#include "gpu/device/include/graphicsStates.h"
#include "gpu/device/include/descriptor.h"
#include "gpu/device/include/shader.h"

#include "core/containers/include/stringBuilder.h"
#include "core/resource/include/staticResource.h"

#include "engine/atlas/include/dynamicGlyphAtlas.h"
#include "engine/atlas/include/dynamicImageAtlas.h"
#include "gpu/device/include/shaderSelector.h"

BEGIN_BOOMER_NAMESPACE()

//---

struct DebugFragmentData
{
    Matrix LocalToWorld;
    Vector4 ConstColor;
    EncodedSelectable Selectable;
};

FrameHelperDebug::FrameHelperDebug(gpu::IDevice* api)
    : m_device(api)
{
    gpu::ShaderSelector selectorSolid;
    selectorSolid.set("SOLID"_id, 1);

    gpu::ShaderSelector selectorTrans;
    selectorTrans.set("TRANSPARENT"_id, 1);

    if (auto shader = gpu::LoadStaticShaderDeviceObject("debugGeometry.fx", selectorSolid))
    {
        {
            gpu::GraphicsRenderStatesSetup setup;
            setup.depth(true);
            setup.depthWrite(true);
            setup.depthFunc(gpu::CompareOp::LessEqual);
            m_renderStatesSolid = shader->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
        }
    }

    if (auto shader = gpu::LoadStaticShaderDeviceObject("debugGeometry.fx", selectorTrans))
    {
        {
            gpu::GraphicsRenderStatesSetup setup;
            setup.depth(true);
            setup.depthWrite(false);
            setup.depthFunc(gpu::CompareOp::LessEqual);
            setup.blend(true);
            setup.blendFactor(0, gpu::BlendFactor::One, gpu::BlendFactor::OneMinusSrcAlpha);
            m_renderStatesTransparent = shader->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
        }

        {
            gpu::GraphicsRenderStatesSetup setup;
            setup.depth(false);
            setup.blend(true);
            setup.blendFactor(0, gpu::BlendFactor::One, gpu::BlendFactor::OneMinusSrcAlpha);
            m_renderStatesOverlay = shader->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
        }
    }
}

FrameHelperDebug::~FrameHelperDebug()
{
}

void FrameHelperDebug::ensureBufferSize(const DebugGeometryCollector* debugGeometry) const
{
    DEBUG_CHECK_RETURN(debugGeometry);

    uint32_t totalVertexDataSize = 0;
    uint32_t totalIndexDataSize = 0;
    debugGeometry->queryLimits(totalVertexDataSize, totalIndexDataSize);

    totalVertexDataSize *= sizeof(DebugVertex);
    totalIndexDataSize *= sizeof(uint32_t);

    if (totalVertexDataSize > m_maxVertexDataSize)
    {
        m_maxVertexDataSize = Align<uint32_t>(totalVertexDataSize, 65536);

        gpu::BufferCreationInfo info;
        info.allowVertex = true;
        info.allowDynamicUpdate = true;
        info.label = "DebugVertexBuffer";
        info.size = m_maxVertexDataSize;
        m_vertexBuffer = m_device->createBuffer(info);
    }

    if (totalIndexDataSize > m_maxIndexDataSize)
    {
        m_maxIndexDataSize = Align<uint32_t>(totalIndexDataSize, 65536);

        gpu::BufferCreationInfo info;
        info.allowIndex = true;
        info.allowDynamicUpdate = true;
        info.label = "DebugIndexBuffer";
        info.size = m_maxIndexDataSize;
        m_indexBuffer = m_device->createBuffer(info);
    }
}

DebugGeometryViewRecorder::DebugGeometryViewRecorder(FrameViewRecorder* parent)
    : FrameViewRecorder(parent)
    , solid(nullptr)
    , transparent(nullptr)
    , overlay(nullptr)
    , screen(nullptr)
{}

void FrameHelperDebug::render(DebugGeometryViewRecorder& rec, const DebugGeometryCollector* debugGeometry, const Camera* camera) const
{
    if (debugGeometry)
    {
        ensureBufferSize(debugGeometry);

        renderInternal(rec.solid, camera, debugGeometry, DebugGeometryLayer::SceneSolid, m_renderStatesSolid);
        renderInternal(rec.transparent, camera, debugGeometry, DebugGeometryLayer::SceneTransparent, m_renderStatesTransparent);
        renderInternal(rec.overlay, camera, debugGeometry, DebugGeometryLayer::Overlay, m_renderStatesOverlay);
        renderInternal(rec.screen, camera, debugGeometry, DebugGeometryLayer::Screen, m_renderStatesOverlay);
    }
}

void FrameHelperDebug::renderInternal(gpu::CommandWriter& cmd, const Camera* camera, const DebugGeometryCollector* debugGeometry, DebugGeometryLayer layer, const gpu::GraphicsPipelineObject* shader) const
{
    gpu::CommandWriterBlock block(cmd, "DebugGeometry");


    // query work
    const DebugGeometryCollector::Element* elemList = nullptr;
    uint32_t elemCount = 0;
    uint32_t elemVertexCount = 0;
    uint32_t elemIndexCount = 0;
    debugGeometry->get(layer, elemList, elemCount, elemVertexCount, elemIndexCount);

    // nothing to draw
    if (!elemList || !shader)
        return;

    // upload vertices
    {
        cmd.opTransitionLayout(m_vertexBuffer, gpu::ResourceLayout::VertexBuffer, gpu::ResourceLayout::CopyDest);
        auto* writePtr = cmd.opUpdateDynamicBufferPtrN<DebugVertex>(m_vertexBuffer, 0, elemVertexCount);

        for (const auto* elem = elemList; elem; elem = elem->next)
        {
            const auto* readV = elem->chunk->vertices();
            const auto* readEndV = readV + elem->chunk->numVertices();

            /*while (readV < readEndV)
            {
                *writePtr = *readV;

                ++readV;
                ++writePtr;
            }*/

            memcpy(writePtr, readV, elem->chunk->numVertices() * sizeof(DebugVertex));
            writePtr += elem->chunk->numVertices();
        }

        cmd.opTransitionLayout(m_vertexBuffer, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::VertexBuffer);
    }

    // upload indices
    {
        cmd.opTransitionLayout(m_indexBuffer, gpu::ResourceLayout::IndexBuffer, gpu::ResourceLayout::CopyDest);
        auto* writePtr = cmd.opUpdateDynamicBufferPtrN<uint32_t>(m_indexBuffer, 0, elemIndexCount);

        uint32_t baseVertex = 0;
        for (const auto* elem = elemList; elem; elem = elem->next)
        {
            const auto* readI = elem->chunk->indices();
            const auto* readEndI = readI + elem->chunk->numIndices();

            while (readI < readEndI)
                *writePtr++ = *readI++ + baseVertex;

            baseVertex += elem->chunk->numVertices();
        }

        cmd.opTransitionLayout(m_indexBuffer, gpu::ResourceLayout::CopyDest, gpu::ResourceLayout::IndexBuffer);
    }

    // bind buffers
    cmd.opBindVertexBuffer("DebugVertex"_id, m_vertexBuffer);
    cmd.opBindIndexBuffer(m_indexBuffer, ImageFormat::R32_UINT);

    // global params
    {
        struct
        {
            Matrix WorldToScreen;
            Vector3 CameraPosition;
            float _padding0;
            Vector2 PixelSizeInScreenUnits;
            Vector2 ProjectZ;
        } data;

        data.WorldToScreen = camera->worldToScreen().transposed();
        data.CameraPosition = camera->position();
        data.PixelSizeInScreenUnits.x = 1.0f / debugGeometry->viewportWidth();
        data.PixelSizeInScreenUnits.y = 1.0f / debugGeometry->viewportHeight();

        // w = sz
        // z = sz*m[2][2] + m[2][3];
        // z' = z / w
        //--
        // z = z' * w
        // z2 += dz
        // w2 = sz2 = (z2 - m[2][3]) / m[2][2];
        // z2 = z2 * w2
        data.ProjectZ.x = camera->viewToScreen().m[2][2];
        data.ProjectZ.y = camera->viewToScreen().m[2][3];

        gpu::DescriptorEntry desc[5];
        desc[0].constants(data);
        desc[1] = GetService<DebugGeometryAssetService>()->imageAtlas()->imageSRV();
        desc[2] = GetService<DebugGeometryAssetService>()->imageAtlas()->imageEntriesSRV();
        desc[3] = GetService<DebugGeometryAssetService>()->glyphAtlas()->imageSRV();
        desc[4] = GetService<DebugGeometryAssetService>()->glyphAtlas()->imageEntriesSRV();
        cmd.opBindDescriptor("DebugFragmentPass"_id, desc);
    }

    // process batches
    uint32_t firstIndex = 0;
    const auto* elem = elemList;
    while (elem)
    {
        const auto* startBatch = elem;
        uint32_t totalIndices = 0;

        while (elem)
        {
            totalIndices += elem->chunk->numIndices();
            elem = elem->next;
        }

        cmd.opDrawIndexed(shader, 0, firstIndex, totalIndices);
        firstIndex += totalIndices;
    }
}

//---

END_BOOMER_NAMESPACE()
