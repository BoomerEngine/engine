/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "gpu/device/include/device.h"
#include "gpu/device/include/commandWriter.h"

BEGIN_BOOMER_NAMESPACE_EX(gpu::test)

/// indexed geometry test
class RenderingTest_IndexedGeometry : public IRenderingTest
{
    RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_IndexedGeometry, IRenderingTest);

public:
    virtual void initialize() override final;
    virtual void render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

private:
    VertexIndexBunch<> m_indexedLineList;
    VertexIndexBunch<> m_indexedTriList;
    VertexIndexBunch<> m_indexedTriStrip;
    VertexIndexBunch<> m_indexedTriStripVertexRestart;

    GraphicsPipelineObjectPtr m_shadersLineList;
	GraphicsPipelineObjectPtr m_shadersTriList;
	GraphicsPipelineObjectPtr m_shadersTriStrip;
	GraphicsPipelineObjectPtr m_shadersTriStripVertexRestart;
};

RTTI_BEGIN_TYPE_CLASS(RenderingTest_IndexedGeometry);
RTTI_METADATA(RenderingTestOrderMetadata).order(80);
RTTI_END_TYPE();

//---       

static const uint32_t GRID_SIZE_X = 64;
static const uint32_t GRID_SIZE_Y = 16;

static Color Rainbow(float x)
{
    static const Color colors[] = {
        Color(148, 0, 211),
            Color(75, 0, 130),
            Color(0, 0, 255),
            Color(0, 255, 0),
            Color(255, 255, 0),
            Color(255, 127, 0),
            Color(255, 0, 0) };

    auto numSegs = ARRAY_COUNT(colors) + 1;
    auto i = (int)floor(fabs(x));
    auto f = fabs(x) - (float)i;

    return LinearInterpolation(f).lerpGamma(colors[i % numSegs], colors[(i + 1) % numSegs]);
}

static void PrepareRainbowVertices(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
{
    for (uint32_t py = 0; py < GRID_SIZE_Y; ++py)
    {
        for (uint32_t px = 0; px < GRID_SIZE_X; ++px)
        {
            auto dv = Vector2(px / (float)GRID_SIZE_X, py / (float)GRID_SIZE_Y);

            Simple3DVertex v;
            v.VertexPosition.x = x + w * dv.x;
            v.VertexPosition.y = y + h * dv.y;
            v.VertexPosition.z = 0.5f;
            v.VertexColor = Rainbow(dv.length() * 17.0f);
            outGeometry.m_vertices.pushBack(v);
        }
    }
}

static void PrepareIndexedLineList(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
{
    PrepareRainbowVertices(x, y, w, h, outGeometry);

    for (uint16_t py = 0; py < GRID_SIZE_Y; ++py)
    {
        uint16_t prevRow = range_cast<uint16_t>((py - 1) * GRID_SIZE_X);
        uint16_t thisRow = range_cast<uint16_t>(py * GRID_SIZE_X);

        for (uint16_t px = 0; px < GRID_SIZE_X; ++px)
        {
            if (px > 0)
            {
                outGeometry.m_indices.pushBack(thisRow + px - 1);
                outGeometry.m_indices.pushBack(thisRow + px);
            }

            if (py > 0)
            {
                outGeometry.m_indices.pushBack(prevRow + px);
                outGeometry.m_indices.pushBack(thisRow + px);
            }
        }
    }
}

static void PrepareIndexedTriangleList(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
{
    PrepareRainbowVertices(x, y, w, h, outGeometry);

    for (uint16_t py = 0; py < GRID_SIZE_Y; ++py)
    {
        uint16_t prevRow = range_cast<uint16_t>((py - 1) * GRID_SIZE_X);
        uint16_t thisRow = range_cast<uint16_t>(py * GRID_SIZE_X);

        for (uint16_t px = 0; px < GRID_SIZE_X; ++px)
        {
            if (px > 0 && py > 0)
            {
                outGeometry.m_indices.pushBack(prevRow + px - 1);
                outGeometry.m_indices.pushBack(prevRow + px);
                outGeometry.m_indices.pushBack(thisRow + px);

                outGeometry.m_indices.pushBack(prevRow + px - 1);
                outGeometry.m_indices.pushBack(thisRow + px);
                outGeometry.m_indices.pushBack(thisRow + px - 1);
            }
        }
    }
}

static void PrepareIndexedTriangleStrip(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
{
    PrepareRainbowVertices(x, y, w, h, outGeometry);

    for (uint16_t py = 0; py < GRID_SIZE_Y; ++py)
    {
        uint16_t prevRow = range_cast<uint16_t>((py - 1) * GRID_SIZE_X);
        uint16_t thisRow = range_cast<uint16_t>(py * GRID_SIZE_X);

        if (py > 1)
            outGeometry.m_indices.pushBack(prevRow);

        for (uint16_t px = 0; px < GRID_SIZE_X; ++px)
        {
            if (py > 0)
            {
                outGeometry.m_indices.pushBack(prevRow + px);
                outGeometry.m_indices.pushBack(thisRow + px);
            }
        }

        if (py > 0)
            outGeometry.m_indices.pushBack(thisRow + GRID_SIZE_X - 1);
    }
}

static void PrepareIndexedTriangleStripWithVertexRestart(float x, float y, float w, float h, VertexIndexBunch<>& outGeometry)
{
    PrepareRainbowVertices(x, y, w, h, outGeometry);

    for (uint16_t py = 0; py < GRID_SIZE_Y; ++py)
    {
        uint16_t prevRow = range_cast<uint16_t>((py - 1) * GRID_SIZE_X);
        uint16_t thisRow = range_cast<uint16_t>(py * GRID_SIZE_X);

        if (py > 1)
            outGeometry.m_indices.pushBack(0xFFFF);

        for (uint16_t px = 0; px < GRID_SIZE_X; ++px)
        {
            if (py > 0)
            {
                outGeometry.m_indices.pushBack(prevRow + px);
                outGeometry.m_indices.pushBack(thisRow + px);
            }
        }
    }
}


void RenderingTest_IndexedGeometry::initialize()
{
	{
		GraphicsRenderStatesSetup setup;
		setup.primitiveTopology(PrimitiveTopology::LineList);
		m_shadersLineList = loadGraphicsShader("GenericGeometry.csl", &setup);
	}

	{
		GraphicsRenderStatesSetup setup;
		setup.primitiveTopology(PrimitiveTopology::TriangleList);
		m_shadersTriList = loadGraphicsShader("GenericGeometry.csl", &setup);
	}

	{
		GraphicsRenderStatesSetup setup;
		setup.primitiveTopology(PrimitiveTopology::TriangleStrip);
		m_shadersTriStrip = loadGraphicsShader("GenericGeometry.csl", &setup);
	}

	{
		GraphicsRenderStatesSetup setup;
		setup.primitiveTopology(PrimitiveTopology::TriangleStrip);
		setup.primitiveRestart(true);
		m_shadersTriStripVertexRestart = loadGraphicsShader("GenericGeometry.csl", &setup);
	}

    float y = -0.9f;
    float ystep = 0.48f;
    float ysize = ystep * 0.9f;

    // create test for indexed lists 
    {
        PrepareIndexedLineList(-0.9f, y, 1.8f, ysize, m_indexedLineList);
        m_indexedLineList.createBuffers(*this);
        y += ystep;
    }

    // create test for triangle list
    {
        PrepareIndexedTriangleList(-0.9f, y, 1.8f, ysize, m_indexedTriList);
        m_indexedTriList.createBuffers(*this);
        y += ystep;
    }

    // create test for triangle strip
    {
        PrepareIndexedTriangleStrip(-0.9f, y, 1.8f, ysize, m_indexedTriStrip);
        m_indexedTriStrip.createBuffers(*this);
        y += ystep;
    }

    // create test for triangle strip with restart vertex
    {
        PrepareIndexedTriangleStripWithVertexRestart(-0.9f, y, 1.8f, ysize, m_indexedTriStripVertexRestart);
        m_indexedTriStripVertexRestart.createBuffers(*this);
        y += ystep;
    }
}

void RenderingTest_IndexedGeometry::render(CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
{
    FrameBuffer fb;
    fb.color[0].view(backBufferView).clear(Vector4(0.0f, 0.0f, 0.2f, 1.0f));

    cmd.opBeingPass(fb);

    m_indexedLineList.draw(cmd, m_shadersLineList);
	m_indexedTriList.draw(cmd, m_shadersTriList);
	m_indexedTriStrip.draw(cmd, m_shadersTriStrip);
    m_indexedTriStripVertexRestart.draw(cmd, m_shadersTriStripVertexRestart);

    cmd.opEndPass();
}

END_BOOMER_NAMESPACE_EX(gpu::test)
