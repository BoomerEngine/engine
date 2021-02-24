/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\helpers #]
***/

#include "build.h"
#include "renderingScene.h"
#include "renderingFrameRenderer.h"
#include "renderingFrameCameraContext.h"
#include "renderingFrameParams.h"
#include "renderingSelectable.h"

#include "renderingFrameHelper_Debug.h"

#include "rendering/device/include/renderingCommandWriter.h"
#include "rendering/device/include/renderingCommandBuffer.h"
#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingDeviceService.h"
#include "rendering/device/include/renderingShaderData.h"
#include "rendering/device/include/renderingGraphicsStates.h"
#include "rendering/device/include/renderingDescriptor.h"
#include "rendering/device/include/renderingShader.h"

#include "base/containers/include/stringBuilder.h"
#include "base/resource/include/resourceStaticResource.h"

BEGIN_BOOMER_NAMESPACE(rendering::scene)

//---

struct DebugFragmentData
{
    base::Matrix LocalToWorld;
    base::Vector4 ConstColor;
    EncodedSelectable Selectable;
};

FrameHelperDebug::FrameHelperDebug(IDevice* api)
    : m_device(api)
{
    m_drawShaderLines = LoadStaticShaderDeviceObject("debugLines.fx");
    m_drawShaderSolid = LoadStaticShaderDeviceObject("debugSolid.fx");

    {
        GraphicsRenderStatesSetup setup;
        setup.depth(true);
        setup.depthWrite(true);
        setup.depthFunc(CompareOp::LessEqual);

        setup.primitiveTopology(PrimitiveTopology::TriangleList);
        m_renderStatesSolid.drawTriangles = m_drawShaderSolid->createGraphicsPipeline(api->createGraphicsRenderStates(setup));

        setup.primitiveTopology(PrimitiveTopology::LineList);
        m_renderStatesSolid.drawLines = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
    }

    {
        GraphicsRenderStatesSetup setup;
        setup.depth(true);
        setup.depthWrite(false);
        setup.depthFunc(CompareOp::LessEqual);
        setup.blend(true);
        setup.blendFactor(0, BlendFactor::One, BlendFactor::OneMinusSrcAlpha);

        setup.primitiveTopology(PrimitiveTopology::TriangleList);
        m_renderStatesTransparent.drawTriangles = m_drawShaderSolid->createGraphicsPipeline(api->createGraphicsRenderStates(setup));

        setup.primitiveTopology(PrimitiveTopology::LineList);
        m_renderStatesTransparent.drawLines = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
    }

    {
        GraphicsRenderStatesSetup setup;
        setup.depth(false);
        setup.blend(true);
        setup.blendFactor(0, BlendFactor::One, BlendFactor::OneMinusSrcAlpha);

        setup.primitiveTopology(PrimitiveTopology::TriangleList);
        m_renderStatesOverlay.drawTriangles = m_drawShaderSolid->createGraphicsPipeline(api->createGraphicsRenderStates(setup));

        setup.primitiveTopology(PrimitiveTopology::LineList);
        m_renderStatesOverlay.drawLines = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
    }

    {
        GraphicsRenderStatesSetup setup;
        setup.depth(false);
        setup.blend(true);
        setup.blendFactor(0, BlendFactor::One, BlendFactor::OneMinusSrcAlpha);

        setup.primitiveTopology(PrimitiveTopology::TriangleList);
        m_renderStatesScreen.drawTriangles = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));

        setup.primitiveTopology(PrimitiveTopology::LineList);
        m_renderStatesScreen.drawLines = m_drawShaderLines->createGraphicsPipeline(api->createGraphicsRenderStates(setup));
    }
}

FrameHelperDebug::~FrameHelperDebug()
{
}

void FrameHelperDebug::ensureBufferSize(const FrameParams_DebugGeometry& geom) const
{
    uint32_t maxVertexSizeThisFrame = 0;
    maxVertexSizeThisFrame = std::max<uint32_t>(maxVertexSizeThisFrame, geom.solid.vertices().dataSize());
    maxVertexSizeThisFrame = std::max<uint32_t>(maxVertexSizeThisFrame, geom.transparent.vertices().dataSize());
    maxVertexSizeThisFrame = std::max<uint32_t>(maxVertexSizeThisFrame, geom.overlay.vertices().dataSize());
    maxVertexSizeThisFrame = std::max<uint32_t>(maxVertexSizeThisFrame, geom.screen.vertices().dataSize());

    if (maxVertexSizeThisFrame > m_maxVertexDataSize)
    {
        m_maxVertexDataSize = base::Align<uint32_t>(maxVertexSizeThisFrame, 65536);

        BufferCreationInfo info;
        info.allowVertex = true;
        info.allowDynamicUpdate = true;
        info.label = "DebugVertexBuffer";
        info.size = m_maxVertexDataSize;
        m_vertexBuffer = m_device->createBuffer(info);
    }

    uint32_t maxIndexSizeThisFrame = 0;
    maxIndexSizeThisFrame = std::max<uint32_t>(maxIndexSizeThisFrame, geom.solid.indices().dataSize());
    maxIndexSizeThisFrame = std::max<uint32_t>(maxIndexSizeThisFrame, geom.transparent.indices().dataSize());
    maxIndexSizeThisFrame = std::max<uint32_t>(maxIndexSizeThisFrame, geom.overlay.indices().dataSize());
    maxIndexSizeThisFrame = std::max<uint32_t>(maxIndexSizeThisFrame, geom.screen.indices().dataSize());

    if (maxIndexSizeThisFrame > m_maxIndexDataSize)
    {
        m_maxIndexDataSize = base::Align<uint32_t>(maxIndexSizeThisFrame, 65536);

        BufferCreationInfo info;
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

void FrameHelperDebug::render(DebugGeometryViewRecorder& rec, const FrameParams_DebugGeometry& geom, const Camera* camera) const
{
    ensureBufferSize(geom);

    renderInternal(rec.solid, camera, geom.solid, m_renderStatesSolid);
    renderInternal(rec.transparent, camera, geom.transparent, m_renderStatesTransparent);
    renderInternal(rec.overlay, camera, geom.overlay, m_renderStatesOverlay);
    renderInternal(rec.screen, camera, geom.screen, m_renderStatesScreen);
}

void FrameHelperDebug::renderInternal(GPUCommandWriter& cmd, const Camera* camera, const DebugGeometry& geom, const Shaders& shaders) const
{
    if (geom.vertices().empty() || geom.indices().empty())
        return;

    CommandWriterBlock block(cmd, "DebugGeometry");

    // upload vertices
    {
        cmd.opTransitionLayout(m_vertexBuffer, ResourceLayout::VertexBuffer, ResourceLayout::CopyDest);
        auto* writePtr = cmd.opUpdateDynamicBufferPtrN<uint8_t>(m_vertexBuffer, 0, geom.vertices().dataSize());
        geom.vertices().copy(writePtr, geom.vertices().dataSize());
        cmd.opTransitionLayout(m_vertexBuffer, ResourceLayout::CopyDest, ResourceLayout::VertexBuffer);
    }

    // upload indices
    {
        cmd.opTransitionLayout(m_indexBuffer, ResourceLayout::IndexBuffer, ResourceLayout::CopyDest);
        auto* writePtr = cmd.opUpdateDynamicBufferPtrN<uint8_t>(m_indexBuffer, 0, geom.indices().dataSize());
        geom.indices().copy(writePtr, geom.indices().dataSize());
        cmd.opTransitionLayout(m_indexBuffer, ResourceLayout::CopyDest, ResourceLayout::IndexBuffer);
    }

    // bind buffers
    cmd.opBindVertexBuffer("DebugVertex"_id, m_vertexBuffer);
    cmd.opBindIndexBuffer(m_indexBuffer, ImageFormat::R32_UINT);

    // global params
    {
        struct
        {
            base::Matrix WorldToScreen;
            base::Vector3 CameraPosition;
            float _padding0;
        } data;

        data.WorldToScreen = camera->worldToScreen().transposed();
        data.CameraPosition = camera->position();

        DescriptorEntry desc[1];
        desc[0].constants(data);
        cmd.opBindDescriptor("DebugFragmentPass"_id, desc);
    }

    // process batches
    const auto* batch = geom.elements().typedData();
    const auto* batchEnd = batch + geom.elements().size();
    while (batch < batchEnd)
    {
        const auto* startBatch = batch++;
        auto expectedNextIndexStart = startBatch->firstIndex + startBatch->numIndices;

        // find end of the range
        if (1)
        {
            while (batch < batchEnd)
            {
                if (batch->firstIndex != expectedNextIndexStart)
                    break;
                /*if (batch->selectable != batch->selectable)
                    break;*/
                if (batch->type != startBatch->type)
                    break;

                expectedNextIndexStart = batch->firstIndex + batch->numIndices;
                batch += 1;
            }
        }

        // draw range
        switch (startBatch->type)
        {
            case DebugGeometryType::Lines:
                cmd.opDrawIndexed(shaders.drawLines, 0, startBatch->firstIndex, expectedNextIndexStart - startBatch->firstIndex);
                break;

            case DebugGeometryType::Solid:
                cmd.opDrawIndexed(shaders.drawTriangles, 0, startBatch->firstIndex, expectedNextIndexStart - startBatch->firstIndex);
                break;
        }
    }
}

//---

END_BOOMER_NAMESPACE(rendering::scene)
