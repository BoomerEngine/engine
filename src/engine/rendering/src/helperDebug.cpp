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
    m_drawShaderLines = gpu::LoadStaticShaderDeviceObject("debugLines.fx");
    m_drawShaderSolid = gpu::LoadStaticShaderDeviceObject("debugGeometry.fx");

    {
        gpu::GraphicsRenderStatesSetup setup;
        setup.depth(true);
        setup.depthWrite(true);
        setup.depthFunc(gpu::CompareOp::LessEqual);

        setup.primitiveTopology(gpu::PrimitiveTopology::TriangleList);
        m_renderStatesSolid.drawTriangles = m_drawShaderSolid->createGraphicsPipeline(api->createGraphicsRenderStates(setup));

        setup.primitiveTopology(gpu::PrimitiveTopology::LineList);
        m_renderStatesSolid.drawLines = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
    }

    {
        gpu::GraphicsRenderStatesSetup setup;
        setup.depth(true);
        setup.depthWrite(false);
        setup.depthFunc(gpu::CompareOp::LessEqual);
        setup.blend(true);
        setup.blendFactor(0, gpu::BlendFactor::One, gpu::BlendFactor::OneMinusSrcAlpha);

        setup.primitiveTopology(gpu::PrimitiveTopology::TriangleList);
        m_renderStatesTransparent.drawTriangles = m_drawShaderSolid->createGraphicsPipeline(api->createGraphicsRenderStates(setup));

        setup.primitiveTopology(gpu::PrimitiveTopology::LineList);
        m_renderStatesTransparent.drawLines = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
    }

    {
        gpu::GraphicsRenderStatesSetup setup;
        setup.depth(false);
        setup.blend(true);
        setup.blendFactor(0, gpu::BlendFactor::One, gpu::BlendFactor::OneMinusSrcAlpha);

        setup.primitiveTopology(gpu::PrimitiveTopology::TriangleList);
        m_renderStatesOverlay.drawTriangles = m_drawShaderSolid->createGraphicsPipeline(api->createGraphicsRenderStates(setup));

        setup.primitiveTopology(gpu::PrimitiveTopology::LineList);
        m_renderStatesOverlay.drawLines = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
    }

    {
        gpu::GraphicsRenderStatesSetup setup;
        setup.depth(false);
        setup.blend(true);
        setup.blendFactor(0, gpu::BlendFactor::One, gpu::BlendFactor::OneMinusSrcAlpha);

        setup.primitiveTopology(gpu::PrimitiveTopology::TriangleList);
        m_renderStatesScreen.drawTriangles = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));

        setup.primitiveTopology(gpu::PrimitiveTopology::LineList);
        m_renderStatesScreen.drawLines = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
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
        renderInternal(rec.screen, camera, debugGeometry, DebugGeometryLayer::Screen, m_renderStatesScreen);
    }
}

void FrameHelperDebug::renderInternal(gpu::CommandWriter& cmd, const Camera* camera, const DebugGeometryCollector* debugGeometry, DebugGeometryLayer layer, const Shaders& shaders) const
{
    gpu::CommandWriterBlock block(cmd, "DebugGeometry");

    // query work
    const DebugGeometryCollector::Element* elemList = nullptr;
    uint32_t elemCount = 0;
    uint32_t elemVertexCount = 0;
    uint32_t elemIndexCount = 0;
    debugGeometry->get(layer, elemList, elemCount, elemVertexCount, elemIndexCount);

    // nothing to draw
    if (!elemList)
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
            Vector2 _padding1;
        } data;

        data.WorldToScreen = camera->worldToScreen().transposed();
        data.CameraPosition = camera->position();
        data.PixelSizeInScreenUnits.x = 1.0f / debugGeometry->viewportWidth();
        data.PixelSizeInScreenUnits.y = 1.0f / debugGeometry->viewportHeight();

        gpu::DescriptorEntry desc[1];
        desc[0].constants(data);
        cmd.opBindDescriptor("DebugFragmentPass"_id, desc);
    }

    // process batches
    uint32_t firstIndex = 0;
    const auto* elem = elemList;
    while (elem)
    {
        const auto* startBatch = elem;
        uint32_t totalIndices = 0;

        // find end of the range
        if (1)
        {
            while (elem)
            {
                /*if (batch->firstIndex != expectedNextIndexStart)
                    break;*/
                /*if (batch->selectable != batch->selectable)
                    break;*/
                /*if (batch->type != startBatch->type)
                    break;*/

                totalIndices += elem->chunk->numIndices();
                elem = elem->next;
            }
        }

        cmd.opDrawIndexed(shaders.drawTriangles, 0, firstIndex, totalIndices);
        firstIndex += totalIndices;
    }
}

//---

END_BOOMER_NAMESPACE()
