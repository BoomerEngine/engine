/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "geometry.h"
#include "service.h"

// The Canvas class is heavily based on nanovg project by Mikko Mononen
// Adaptations were made to fit the rest of the source code in here

//
// Copyright (c) 2013 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

BEGIN_BOOMER_NAMESPACE()

//---

bool CanvasAttributes::operator==(const CanvasAttributes& other) const
{
	return 0 == memcmp(this, &other, sizeof(other));
}

bool CanvasAttributes::operator!=(const CanvasAttributes& other) const
{
	return !operator==(other);
}

uint32_t CanvasAttributes::CalcHash(const CanvasAttributes& style)
{
	return CRC32().append(&style, sizeof(style));
}

//---

CanvasGeometry::CanvasGeometry()
	: boundsMin(FLT_MAX, FLT_MAX)
	, boundsMax(-FLT_MAX, -FLT_MAX)
{}

CanvasGeometry::~CanvasGeometry()
{
}

CanvasGeometry::CanvasGeometry(const CanvasGeometry& other)
	: boundsMin(other.boundsMin)
	, boundsMax(other.boundsMax)
	, batches(other.batches)
	, vertices(other.vertices)
	, attributes(other.attributes)
{}

CanvasGeometry::CanvasGeometry(CanvasGeometry&& other)
	: boundsMin(other.boundsMin)
	, boundsMax(other.boundsMax)
	, batches(std::move(other.batches))
	, vertices(std::move(other.vertices))
	, attributes(std::move(other.attributes))
{
	other.boundsMin = Vector2(FLT_MAX, FLT_MAX);
	other.boundsMax = Vector2(-FLT_MAX, -FLT_MAX);
}

CanvasGeometry& CanvasGeometry::operator=(const CanvasGeometry& other)
{
	if (this != &other)
	{
		boundsMin = other.boundsMin;
		boundsMax = other.boundsMax;
		vertices = other.vertices;
		batches = other.batches;
		attributes = other.attributes;
	}

	return *this;
}

CanvasGeometry& CanvasGeometry::operator=(CanvasGeometry&& other)
{
	if (this != &other)
	{
		vertices = std::move(other.vertices);
		batches = std::move(other.batches);
		attributes = std::move(other.attributes);

		boundsMin = other.boundsMin;
		boundsMax = other.boundsMax;
		other.boundsMin = Vector2(FLT_MAX, FLT_MAX);
		other.boundsMax = Vector2(-FLT_MAX, -FLT_MAX);
	}

	return *this;
}

void CanvasGeometry::reset()
{
    boundsMin = Vector2(FLT_MAX, FLT_MAX);
    boundsMax = Vector2(-FLT_MAX, -FLT_MAX);

	attributes.reset();
    vertices.reset();
	batches.reset();
}

uint32_t CanvasGeometry::calcMemorySize() const
{
	uint32_t ret = 0;
	ret += vertices.dataSize();
	ret += batches.dataSize();
	ret += attributes.dataSize();
	ret += customData.dataSize();
	return ret;
}

//--

extern void BuildAttributesFromStyle(const CanvasRenderStyle& style, float width, CanvasAttributes& outAttributes);

int CanvasGeometry::appendStyle(const CanvasRenderStyle& style)
{
	int styleIndex = 0;
	if (!style.attributesNeeded)
		return 0;

	CanvasAttributes attr;
	BuildAttributesFromStyle(style, 1.0f, attr);

	for (auto i : attributes.indexRange())
		if (attributes[i] == attr)
			return i + 1;

	attributes.pushBack(attr);
	return attributes.size();			
}

void CanvasGeometry::applyStyle(CanvasVertex* vertices, uint32_t numVertices, const CanvasRenderStyle& style)
{			
	auto* vertexPtr = vertices + numVertices;

	static const auto* service = GetService<CanvasService>();

	auto attributesIndex = appendStyle(style);
	/*if (attributesIndex == 0)
	{
		auto* vertex = vertices;
		while (vertex < vertexPtr)
		{
			vertex->color = style.innerColor;
			++vertex;
		}
	}
	else*/

	{
		auto flags = CanvasVertex::MASK_FILL;

		auto imageEntryIndex = 0;
		if (style.image)
		{
			flags |= CanvasVertex::MASK_HAS_IMAGE;
			imageEntryIndex = style.image->id();
		}

		auto* vertex = vertices;
		while (vertex < vertexPtr)
		{
			vertex->attributeIndex = attributesIndex;
			vertex->attributeFlags = flags;
			vertex->imageEntryIndex = imageEntryIndex;
			vertex->imagePageIndex = 0;
			++vertex;
		}
	}

	// compute UV from style
	if (!style.customUV)
	{
		auto* vertex = vertices;
		while (vertex < vertexPtr)
		{
			vertex->uv.x = style.xform.transformX(vertex->pos.x, vertex->pos.y);
			vertex->uv.y = style.xform.transformX(vertex->pos.x, vertex->pos.y);
			++vertex;
		}
	}
}

void CanvasGeometry::appendVertexBatch(const CanvasVertex* vertices, uint32_t numVertices, const CanvasBatch& setup, const CanvasRenderStyle* style)
{
	if (!vertices || !numVertices)
		return;

	auto firstVertexIndex = this->vertices.size();
	auto* localVertices = this->vertices.allocateUninitialized(numVertices);
	memcpy(localVertices, vertices, sizeof(CanvasVertex) * numVertices);

	if (style)
		applyStyle(localVertices, numVertices, *style);

	auto& batch = batches.emplaceBack(setup);
	batch.vertexOffset = firstVertexIndex;
}

void CanvasGeometry::appendIndexedBatch(const CanvasVertex* vertices, const uint16_t* indices, uint32_t numIndices, const CanvasBatch& setup, const CanvasRenderStyle* style)
{
	if (!vertices || !indices || !numIndices)
		return;

	auto firstVertexIndex = this->vertices.size();
	auto* localVertices = this->vertices.allocateUninitialized(numIndices);
	{
		auto* vertex = localVertices;
		const auto* readPtr = indices;
		const auto* readEndPtr = indices + numIndices;
		while (readPtr < readEndPtr)
			*vertex++ = vertices[*readPtr++];
	}

	if (style)
		applyStyle(localVertices, numIndices, *style);

	auto& batch = batches.emplaceBack(setup);
	batch.vertexOffset = firstVertexIndex;
	batch.vertexCount = numIndices;
}

//--

END_BOOMER_NAMESPACE()
