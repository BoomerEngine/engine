/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"

#include "canvas.h"
#include "service.h"
#include "geometry.h"
#include "geometryBuilder.h"

#include "engine/font/include/fontGlyphBuffer.h"
#include "engine/font/include/fontInputText.h"
#include "engine/font/include/fontGlyph.h"

BEGIN_BOOMER_NAMESPACE()

//--

INLINE bool TestClip(const Vector4& clipRect, const Vector2& boundsMin, const Vector2& boundsMax)
{
    if (boundsMin.x > clipRect.z || boundsMin.y > clipRect.w)
        return false;
    if (boundsMax.x < clipRect.x || boundsMax.y < clipRect.y)
        return false;
    return true;
}

INLINE bool TestClip(const Vector4& clipRect, float minX, float minY, float maxX, float maxY)
{
    if (minX > clipRect.z || minY > clipRect.w)
        return false;
    if (maxX < clipRect.x || maxY < clipRect.y)
        return false;
    return true;
}

//--

Canvas::Canvas(const Setup& setup)
    : m_width(setup.width)
    , m_height(setup.height)
    , m_pixelOffset(setup.pixelOffset)
    , m_pixelScale(setup.pixelScale)
    , m_clearColor(0,0,0,0) // 0 alpha - not used
{
	m_pixelOffset.x = std::round(m_pixelOffset.x);
	m_pixelOffset.y = std::round(m_pixelOffset.y);
	m_scissorRect = Vector4(0.0f, 0.0f, m_width, m_height);

	updatePixelTransform();
    updateScissorRect();
}

Canvas::Canvas(uint32_t width, uint32_t height)
	: Canvas(Setup{ width, height })
{			
}

Canvas::~Canvas()
{
}

void Canvas::updatePixelTransform()
{
    m_pixelTransform.t[4] += m_pixelOffset.x;
    m_pixelTransform.t[5] += m_pixelOffset.y;
    m_pixelTransform.t[0] *= m_pixelScale;
    m_pixelTransform.t[1] *= m_pixelScale;
    m_pixelTransform.t[2] *= m_pixelScale;
    m_pixelTransform.t[3] *= m_pixelScale;
}

void Canvas::updateScissorRect()
{
    const auto prevRect = m_pixelScissorShaderValues;

    m_pixelScissorRect.x = (m_scissorRect.x + m_pixelOffset.x) * m_pixelScale;
    m_pixelScissorRect.y = (m_scissorRect.y + m_pixelOffset.y) * m_pixelScale;
    m_pixelScissorRect.z = (m_scissorRect.z + m_pixelOffset.x) * m_pixelScale;
    m_pixelScissorRect.w = (m_scissorRect.w + m_pixelOffset.y) * m_pixelScale;

    if (m_pixelScissorRect.z >= (m_pixelScissorRect.x + 1.0f) && m_pixelScissorRect.w > (m_pixelScissorRect.y + 1.0f))
    {
        m_emptyScissorRect = false;

        float extentsX = (m_pixelScissorRect.z - m_pixelScissorRect.x) * 0.5f;
        float extentsY = (m_pixelScissorRect.w - m_pixelScissorRect.y) * 0.5f;

        m_pixelScissorShaderValues.x = -(m_pixelScissorRect.x + extentsX);
        m_pixelScissorShaderValues.y = -(m_pixelScissorRect.y + extentsY);
        m_pixelScissorShaderValues.z = 1.0f / extentsX;
        m_pixelScissorShaderValues.w = 1.0f / extentsY;
    }
    else
    {
        m_emptyScissorRect = true;
        m_pixelScissorShaderValues.x = 0.0f;
        m_pixelScissorShaderValues.y = 0.0f;
        m_pixelScissorShaderValues.z = 0.0f;
        m_pixelScissorShaderValues.w = 0.0f;
    }
}

//--

void Canvas::pixelPlacement(float pixelOffsetX, float pixelOffsetY, float pixelScale /*= 1.0f*/)
{
    if (m_pixelOffset.x != pixelOffsetX || m_pixelOffset.y != pixelOffsetY || m_pixelScale != pixelScale)
    {
        m_pixelOffset.x = pixelOffsetX;
        m_pixelOffset.y = pixelOffsetY;
        m_pixelScale = pixelScale;
                
        updatePixelTransform();
        updateScissorRect();
    }
}

//--

Canvas::DebugTextBounds Canvas::debugPrintMeasure(StringView text, int size /*= 16*/, int align /*= -1*/, bool bold /*= false*/) const
{
    static const auto normalFont = LoadFontFromDepotPath("/engine/interface/fonts/aileron_regular.otf");
    static const auto boldFont = LoadFontFromDepotPath("/engine/interface/fonts/aileron_bold.otf");

    auto font = bold ? boldFont : normalFont;

	if (font)
	{
		FontStyleParams params;
		params.size = size;

		FontGlyphBuffer glyphs;
        font->renderText(params, text, glyphs);

		DebugTextBounds bounds;

		if (!glyphs.m_glyphs.empty())
		{
            bounds.min = Vector2::INF();
			bounds.max = -Vector2::INF();

			for (const auto& g : glyphs.m_glyphs)
			{
				float x0 = g.pos.x + g.glyph->offset().x;
				float y0 = g.pos.y + g.glyph->offset().y;
                float x1 = x0 + g.glyph->size().x;
                float y1 = y0 + g.glyph->size().y;

				bounds.min.x = std::min(bounds.min.x, x0);
				bounds.min.y = std::min(bounds.min.y, y0);
                bounds.max.x = std::max(bounds.max.x, x1);
                bounds.max.y = std::max(bounds.max.y, y1);
			}
		}

		bounds.size.x = bounds.max.x - bounds.min.x;
		bounds.min.y = font->relativeAscender() * size;
		bounds.max.y = font->relativeDescender() * size;
		bounds.size.y = font->relativeLineHeight() * size;
		return bounds;
	}

	return DebugTextBounds();
}

Canvas::DebugTextBounds Canvas::debugPrint(float x, float y, StringView text, Color color /*= Color::WHITE*/, int size /*= 16*/, int align /*= FontAlignmentHorizontal::Left*/, bool bold /*= false*/)
{
    static const auto normalFont = LoadFontFromDepotPath("/engine/interface/fonts/aileron_regular.otf");
	static const auto boldFont = LoadFontFromDepotPath("/engine/interface/fonts/aileron_bold.otf");
			
	auto font = bold ? boldFont : normalFont;

	if (font)
	{
		FontStyleParams params;
		params.size = size;

		FontGlyphBuffer glyphs;
		font->renderText(params, text, glyphs, color);

		if (align >= 0)
		{
			Rect bounds;
			if (glyphs.bounds(bounds))
			{
				if (align == 0)
					x -= bounds.width() / 2;
				else if (align > 0)
					x -= bounds.width();
			}
		}

		CanvasGeometry g;
		{
			CanvasGeometryBuilder b(g);
			b.print(glyphs);
		}

		place(x, y, g);

		DebugTextBounds bounds;
		bounds.min.x = g.boundsMin.x;
		bounds.max.x = g.boundsMax.x;
        bounds.size.x = bounds.max.x - bounds.min.x;
        bounds.min.y = font->relativeAscender() * size;
        bounds.max.y = font->relativeDescender() * size;
        bounds.size.y = font->relativeLineHeight() * size;
		return bounds;
	}

	return DebugTextBounds();
}

//--

void Canvas::clearScissorRectStack()
{
    m_scissorRectStack.clear();
    updateScissorRect();
}

void Canvas::resetScissorRect()
{
    m_scissorRect = Vector4(0.0f, 0.0f, m_width, m_height);
    updateScissorRect();
}

void Canvas::pushScissorRect()
{
	m_scissorRectStack.emplaceBack(m_scissorRect);
}

void Canvas::popScissorRect()
{
    if (!m_scissorRectStack.empty())
    {
		m_scissorRect = m_scissorRectStack.back();
        m_scissorRectStack.popBack();
        updateScissorRect();
    }
}

bool Canvas::scissorRect(const Vector2& position, const Vector2& extents)
{
    m_scissorRect = Vector4(position.x, position.y, position.x + extents.x, position.y + extents.y);
    updateScissorRect();
    return !m_emptyScissorRect;
}

bool Canvas::scissorRect(float x, float y, float w, float h)
{
    m_scissorRect = Vector4(x, y, x + w, y + h);
    updateScissorRect();
    return !m_emptyScissorRect;
}

bool Canvas::scissorBounds(const Vector2& bmin, const Vector2& bmax)
{
    m_scissorRect = Vector4(bmin.x, bmin.y, bmax.x, bmax.y);
    updateScissorRect();
    return !m_emptyScissorRect;
}

bool Canvas::scissorBounds(float x0, float y0, float x1, float y1)
{
    m_scissorRect = Vector4(x0, y0, x1, y1);
    updateScissorRect();
    return !m_emptyScissorRect;
}

bool Canvas::intersectScissorRect(const Vector2& position, const Vector2& extents)
{
    m_scissorRect.x = std::max<float>(m_scissorRect.x, position.x);
    m_scissorRect.y = std::max<float>(m_scissorRect.y, position.y);
    m_scissorRect.z = std::min<float>(m_scissorRect.z, position.x + extents.x);
    m_scissorRect.w = std::min<float>(m_scissorRect.w, position.y + extents.y);
    updateScissorRect();
    return !m_emptyScissorRect;
}

bool Canvas::intersectScissorRect(float x, float y, float w, float h)
{
    m_scissorRect.x = std::max<float>(m_scissorRect.x, x);
    m_scissorRect.y = std::max<float>(m_scissorRect.y, y);
    m_scissorRect.z = std::min<float>(m_scissorRect.z, x+w);
    m_scissorRect.w = std::min<float>(m_scissorRect.w, y+h);
    updateScissorRect();
    return !m_emptyScissorRect;
}

bool Canvas::intersectScissorBounds(const Vector2& bmin, const Vector2& bmax)
{
    m_scissorRect.x = std::max<float>(m_scissorRect.x, bmin.x);
    m_scissorRect.y = std::max<float>(m_scissorRect.y, bmin.y);
    m_scissorRect.z = std::min<float>(m_scissorRect.z, bmax.x);
    m_scissorRect.w = std::min<float>(m_scissorRect.w, bmax.y);
    updateScissorRect();
    return !m_emptyScissorRect;
}

bool Canvas::intersectScissorBounds(float x0, float y0, float x1, float y1)
{
    m_scissorRect.x = std::max<float>(m_scissorRect.x, x0);
    m_scissorRect.y = std::max<float>(m_scissorRect.y, y0);
    m_scissorRect.z = std::min<float>(m_scissorRect.z, x1);
    m_scissorRect.w = std::min<float>(m_scissorRect.w, y1);
    updateScissorRect();
    return !m_emptyScissorRect;
}

bool Canvas::testScissorRect(const Vector2& position, const Vector2& extents) const
{
    return TestClip(m_scissorRect, position.x, position.y, position.x + extents.x, position.y + extents.y);
}

bool Canvas::testScissorRect(float x, float y, float w, float h) const
{
    return TestClip(m_scissorRect, x, y, x + w, y + h);
}

bool Canvas::testScissorBounds(const Vector2& bmin, const Vector2& bmax) const
{
    return TestClip(m_scissorRect, bmin.x, bmin.y, bmax.x, bmax.y);
}

bool Canvas::testScissorBounds(float x0, float y0, float x1, float y1) const
{
    return TestClip(m_scissorRect, x0, y0, x1, y1);
}

//--

void Canvas::transformBounds(const CanvasPlacement& transform, const Vector2& localMin, const Vector2& localMax, Vector2& globalMin, Vector2& globalMax) const
{
	if (transform.simple)
	{
		if (transform.noScale)
		{
			globalMin.x = localMin.x + transform.placement.t[4];
			globalMin.y = localMin.y + transform.placement.t[5];
			globalMax.x = localMax.x + transform.placement.t[4];
			globalMax.y = localMax.y + transform.placement.t[5];
		}
		else
		{
			globalMin.x = (localMin.x * transform.placement.t[0]) + transform.placement.t[4];
			globalMin.y = (localMin.y * transform.placement.t[0]) + transform.placement.t[5];
			globalMax.x = (localMax.x * transform.placement.t[0]) + transform.placement.t[4];
			globalMax.y = (localMax.y * transform.placement.t[0]) + transform.placement.t[5];
		}
	}
	else
	{
		globalMax = globalMin = transform.placement.transformPoint(localMin);

		{
			auto v = transform.placement.transformPoint(localMax);
			globalMax = globalMax.min(v);
			globalMin = globalMin.max(v);
		}

		{
			auto v = transform.placement.transformPoint(Vector2(localMax.x, localMin.y));
            globalMax = globalMax.min(v);
            globalMin = globalMin.max(v);
		}

		{
			auto v = transform.placement.transformPoint(Vector2(localMin.x, localMax.y));
            globalMax = globalMax.min(v);
            globalMin = globalMin.max(v);
		}
	}
}

void Canvas::place(float x, float y, const CanvasGeometry& geometry, float alpha /*= 1.0f*/)
{
	place(CanvasPlacement(x, y), geometry, alpha);
}

void Canvas::place(const CanvasPlacement& placement, const CanvasGeometry& geometry, float alpha)
{
	if (alpha > 0.0f)
	{
		Vector2 globalMin, globalMax;
		transformBounds(placement, geometry.boundsMin, geometry.boundsMax, globalMin, globalMax);

		//if (TestClip(m_scissorRect, globalMax, globalMax))
		{
			const auto firstAttributeIndex = m_gatheredAttributes.size();
			if (!geometry.attributes.empty())
			{
				auto* writePtr = m_gatheredAttributes.allocateUninitialized(geometry.attributes.size());
				memcpy(writePtr, geometry.attributes.typedData(), geometry.attributes.dataSize());
			}

			const auto firstDataOffset = m_gatheredData.size();
			if (!geometry.customData.empty())
			{
				auto writeSize = Align<uint32_t>(16, geometry.customData.dataSize());
				auto* writePtr = m_gatheredData.allocateUninitialized(writeSize);
				memcpy(writePtr, geometry.customData.typedData(), geometry.customData.dataSize());
			}

			const auto* vertices = geometry.vertices.typedData();
			for (const auto& batch : geometry.batches)
				placeInternal(placement, vertices + batch.vertexOffset, batch.vertexCount, firstAttributeIndex, firstDataOffset, batch, alpha);
		}
	}
}

static uint32_t CalcFlattenedVertexCount(uint32_t numVertices, CanvasBatchPacking packing)
{
	switch (packing)
	{
		case CanvasBatchPacking::TriangleList:
			ASSERT(numVertices >= 3 && (numVertices % 3) == 0);
			return numVertices;
				
		case CanvasBatchPacking::TriangleStrip:
		case CanvasBatchPacking::TriangleFan:
			ASSERT(numVertices >= 3);
			return (numVertices - 2) * 3;

		case CanvasBatchPacking::Quads:
			ASSERT(numVertices >= 4 && (numVertices % 4) == 0);
			return (numVertices / 4) * 6;
	}

	ASSERT(!"Invalid packing mode");
	return 0;
}

void Canvas::placeVertex(const CanvasPlacement& placement, const CanvasVertex* vertexPtr, CanvasVertex*& writePtr, uint32_t firstAttributeIndex)
{
	if (placement.simple)
	{
		writePtr->pos.x = (vertexPtr->pos.x * placement.placement.t[0] + placement.placement.t[4]) * m_pixelScale + m_pixelOffset.x;
		writePtr->pos.y = (vertexPtr->pos.y * placement.placement.t[0] + placement.placement.t[5]) * m_pixelScale + m_pixelOffset.y;
	}
	else
	{
		writePtr->pos.x = placement.placement.transformX(vertexPtr->pos.x, vertexPtr->pos.y) * m_pixelScale + m_pixelOffset.x;
		writePtr->pos.y = placement.placement.transformY(vertexPtr->pos.x, vertexPtr->pos.y) * m_pixelScale + m_pixelOffset.y;
	}

	writePtr->uv.x = vertexPtr->uv.x;
	writePtr->uv.y = vertexPtr->uv.y;
	writePtr->clipUV.x = (writePtr->pos.x + m_pixelScissorShaderValues.x) * m_pixelScissorShaderValues.z;
	writePtr->clipUV.y = (writePtr->pos.y + m_pixelScissorShaderValues.y) * m_pixelScissorShaderValues.w;
	writePtr->color = vertexPtr->color;
	writePtr->attributeFlags = vertexPtr->attributeFlags;
	writePtr->attributeIndex = vertexPtr->attributeIndex ? (vertexPtr->attributeIndex + firstAttributeIndex) : 0;
	writePtr->imageEntryIndex = vertexPtr->imageEntryIndex;
	writePtr->imagePageIndex = vertexPtr->imagePageIndex;

	++writePtr;
}

void Canvas::placeInternal(const CanvasPlacement& placement, const CanvasVertex* vertices, uint32_t numVertices, uint32_t firstAttributeIndex, uint32_t firstDataOffset, const CanvasBatch& batch, float alpha)
{
	// nothing to place
	if (numVertices <= 2 || !vertices)
		return;

	// compute target vertex count
	const auto targetVertexCount = CalcFlattenedVertexCount(numVertices, batch.packing);
	DEBUG_CHECK_RETURN_EX(targetVertexCount <= MAX_LOCAL_VERTICES, "To many vertices in one batch");

	// reader and writer pointers
	auto* writePtr = m_tempVertices;
	const auto* writeEndPtr = writePtr + targetVertexCount;
	const auto* readPtr = vertices;
	const auto* readEndPtr = vertices + numVertices;

	// process topology
	switch (batch.packing)
	{
		case CanvasBatchPacking::TriangleList:
		{
			while (readPtr < readEndPtr)
				placeVertex(placement, readPtr++, writePtr, firstAttributeIndex);
			break;
		}

		case CanvasBatchPacking::Quads:
		{
			while (readPtr < readEndPtr)
			{
				ASSERT(readPtr + 4 <= readEndPtr);

				auto* quadPtr = writePtr;
				placeVertex(placement, readPtr + 0, writePtr, firstAttributeIndex);
				placeVertex(placement, readPtr + 1, writePtr, firstAttributeIndex);
				placeVertex(placement, readPtr + 2, writePtr, firstAttributeIndex);
				placeVertex(placement, readPtr + 0, writePtr, firstAttributeIndex);
				placeVertex(placement, readPtr + 2, writePtr, firstAttributeIndex);
				placeVertex(placement, readPtr + 3, writePtr, firstAttributeIndex);

				float shiftX = quadPtr[0].pos.x - std::truncf(quadPtr[0].pos.x) + 0.5f;
				float shiftY = quadPtr[0].pos.y - std::truncf(quadPtr[0].pos.y) + 0.5f;
				quadPtr[0].pos.x -= shiftX;
				quadPtr[0].pos.y -= shiftY;
				quadPtr[1].pos.x -= shiftX;
				quadPtr[1].pos.y -= shiftY;
				quadPtr[2].pos.x -= shiftX;
				quadPtr[2].pos.y -= shiftY;
				quadPtr[3].pos.x -= shiftX;
				quadPtr[3].pos.y -= shiftY;
				quadPtr[4].pos.x -= shiftX;
				quadPtr[4].pos.y -= shiftY;
				quadPtr[5].pos.x -= shiftX;
				quadPtr[5].pos.y -= shiftY;

				readPtr += 4;
			}
			break;
		}

		case CanvasBatchPacking::TriangleFan:
		{
			const auto* firstPtr = readPtr + 0;
			const auto* prevPtr = readPtr + 1;
			readPtr += 2;
			while (readPtr < readEndPtr)
			{
				placeVertex(placement, firstPtr, writePtr, firstAttributeIndex);
				placeVertex(placement, prevPtr, writePtr, firstAttributeIndex);
				placeVertex(placement, readPtr, writePtr, firstAttributeIndex);
				prevPtr = readPtr;
				readPtr += 1;
			}
			break;
		}

		case CanvasBatchPacking::TriangleStrip:
		{
			const auto* basePtr = readPtr + 0;
			const auto* prevPtr = readPtr + 1;
			readPtr += 2;
			bool winding = true;
			while (readPtr < readEndPtr)
			{
				if (winding)
				{
					placeVertex(placement, basePtr, writePtr, firstAttributeIndex);
					placeVertex(placement, prevPtr, writePtr, firstAttributeIndex);
					placeVertex(placement, readPtr, writePtr, firstAttributeIndex);
				}
				else
				{
					placeVertex(placement, prevPtr, writePtr, firstAttributeIndex);
					placeVertex(placement, basePtr, writePtr, firstAttributeIndex);
					placeVertex(placement, readPtr, writePtr, firstAttributeIndex);
				}

				winding = !winding;
				basePtr = prevPtr;
				prevPtr = readPtr;
				readPtr += 1;
			}
			break;
		}
	}

	// make sure all was written as expected
	ASSERT(writePtr == writeEndPtr);

	// place batch
	auto& outBatch = m_gatheredBatches.emplaceBack();
	outBatch.type = batch.type;
	outBatch.rendererIndex = batch.rendererIndex;
	outBatch.op = batch.op;
	outBatch.vertexOffset = m_gatheredVertices.size();
	outBatch.vertexCount = targetVertexCount;
	outBatch.renderDataOffset = firstDataOffset + batch.renderDataOffset;
	outBatch.renderDataSize = batch.renderDataSize;

	// push vertices as well
	m_gatheredVertices.writeLarge(m_tempVertices, sizeof(CanvasVertex)* targetVertexCount);
}		

//---

void Canvas::quad(const CanvasPlacement& placement, const QuadSetup& setup, uint8_t customRenderer /*= 0*/, const void* customData /*= nullptr*/, uint32_t customDataSize /*= 0*/)
{
	CanvasVertex v[4];
	v[0].pos.x = (int)std::round(setup.x0);
	v[0].pos.y = (int)std::round(setup.y0);
	v[1].pos.x = (int)std::round(setup.x1);
	v[1].pos.y = (int)std::round(setup.y0);
	v[2].pos.x = (int)std::round(setup.x1);
	v[2].pos.y = (int)std::round(setup.y1);
	v[3].pos.x = (int)std::round(setup.x0);
	v[3].pos.y = (int)std::round(setup.y1);

	static const auto* service = GetService<CanvasService>();

	if (setup.image)
	{ 
		v[0].uv.x = setup.u0;
		v[0].uv.y = setup.v0;
		v[1].uv.x = setup.u1;
		v[1].uv.y = setup.v0;
		v[2].uv.x = setup.u1;
		v[2].uv.y = setup.v1;
		v[3].uv.x = setup.u0;
		v[3].uv.y = setup.v1;

		auto flags = CanvasVertex::MASK_HAS_IMAGE | CanvasVertex::MASK_FILL;
		if (setup.image->wrapU())
			flags |= CanvasVertex::MASK_HAS_WRAP_U;
        if (setup.image->wrapV())
            flags |= CanvasVertex::MASK_HAS_WRAP_V;

		for (int i = 0; i < 4; ++i)
		{
			v[i].attributeIndex = 0;
			v[i].attributeFlags = flags;
			v[i].imageEntryIndex = setup.image->id();
			v[i].imagePageIndex = 0; // TODO: remove
			v[i].color = setup.color;
		}
	}
	else
	{
		v[0].uv.x = setup.u0;
		v[0].uv.y = setup.v0;
		v[1].uv.x = setup.u1;
		v[1].uv.y = setup.v0;
		v[2].uv.x = setup.u1;
		v[2].uv.y = setup.v1;
		v[3].uv.x = setup.u0;
		v[3].uv.y = setup.v1;

		auto flags = CanvasVertex::MASK_FILL;

		for (int i = 0; i < 4; ++i)
		{
			v[i].attributeIndex = 0;
			v[i].attributeFlags = flags;
			v[i].imageEntryIndex = 0;
			v[i].imagePageIndex = 0;
			v[i].color = setup.color;
		}
	}

	CanvasBatch batch;
	batch.rendererIndex = customRenderer;
	batch.type = CanvasBatchType::FillConvex;
	batch.packing = CanvasBatchPacking::Quads;
	batch.op = setup.op;

	if (customDataSize > 0 && customData)
	{
		batch.renderDataSize = customDataSize;
		batch.renderDataOffset = m_gatheredData.size();
		memcpy(m_gatheredData.allocateUninitialized(customDataSize), customData, customDataSize);
	}

	placeInternal(placement, v, 4, 0, 0, batch, 1.0f);
}

//---
        
END_BOOMER_NAMESPACE()
