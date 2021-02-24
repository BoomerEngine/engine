/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"
#include "canvasGeometry.h"
#include "canvasService.h"

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

BEGIN_BOOMER_NAMESPACE(base::canvas)

//---

bool Attributes::operator==(const Attributes& other) const
{
	return 0 == memcmp(this, &other, sizeof(other));
}

bool Attributes::operator!=(const Attributes& other) const
{
	return !operator==(other);
}

uint32_t Attributes::CalcHash(const Attributes& style)
{
	return CRC32().append(&style, sizeof(style));
}

//---

Geometry::Geometry()
	: boundsMin(FLT_MAX, FLT_MAX)
	, boundsMax(-FLT_MAX, -FLT_MAX)
{}

Geometry::~Geometry()
{
}

Geometry::Geometry(const Geometry& other)
	: boundsMin(other.boundsMin)
	, boundsMax(other.boundsMax)
	, batches(other.batches)
	, vertices(other.vertices)
	, attributes(other.attributes)
{}

Geometry::Geometry(Geometry&& other)
	: boundsMin(other.boundsMin)
	, boundsMax(other.boundsMax)
	, batches(std::move(other.batches))
	, vertices(std::move(other.vertices))
	, attributes(std::move(other.attributes))
{
	other.boundsMin = Vector2(FLT_MAX, FLT_MAX);
	other.boundsMax = Vector2(-FLT_MAX, -FLT_MAX);
}

Geometry& Geometry::operator=(const Geometry& other)
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

Geometry& Geometry::operator=(Geometry&& other)
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

void Geometry::reset()
{
    boundsMin = Vector2(FLT_MAX, FLT_MAX);
    boundsMax = Vector2(-FLT_MAX, -FLT_MAX);

	attributes.reset();
    vertices.reset();
	batches.reset();
}

uint32_t Geometry::calcMemorySize() const
{
	uint32_t ret = 0;
	ret += vertices.dataSize();
	ret += batches.dataSize();
	ret += attributes.dataSize();
	ret += customData.dataSize();
	return ret;
}

//--

extern void BuildAttributesFromStyle(const RenderStyle& style, float width, Attributes& outAttributes);

int Geometry::appendStyle(const RenderStyle& style)
{
	int styleIndex = 0;
	if (!style.attributesNeeded)
		return 0;

	Attributes attr;
	BuildAttributesFromStyle(style, 1.0f, attr);

	for (auto i : attributes.indexRange())
		if (attributes[i] == attr)
			return i + 1;

	attributes.pushBack(attr);
	return attributes.size();			
}

void Geometry::applyStyle(Vertex* vertices, uint32_t numVertices, const RenderStyle& style)
{			
	auto* vertexPtr = vertices + numVertices;

	static const auto* service = GetService<CanvasService>();
	const auto* imageEntry = style.image ? service->findRenderDataForAtlasEntry(style.image) : nullptr;

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
		auto flags = Vertex::MASK_FILL;

		int imagePage = 0;
		if (imageEntry)
		{
			flags |= Vertex::MASK_HAS_IMAGE;
			imagePage = imageEntry->pageIndex;
		}

		auto* vertex = vertices;
		while (vertex < vertexPtr)
		{
			vertex->attributeIndex = attributesIndex;
			vertex->attributeFlags = flags;
			vertex->imageEntryIndex = style.image.entryIndex;
			vertex->imagePageIndex = imagePage;
			++vertex;
		}
	}

	if (style.customUV)
	{
		if (imageEntry)
		{
			auto uvScale = imageEntry->uvMax - imageEntry->uvOffset;

			auto* vertex = vertices;
			while (vertex < vertexPtr)
			{
				vertex->uv.x = (vertex->uv.x * uvScale.x) + imageEntry->uvOffset.x;
				vertex->uv.y = (vertex->uv.y * uvScale.y) + imageEntry->uvOffset.y;
				++vertex;
			}
		}
	}
	else if (imageEntry)
	{
		auto* vertex = vertices;
		while (vertex < vertexPtr)
		{
			vertex->uv.x = (style.xform.transformX(vertex->pos.x, vertex->pos.y) * imageEntry->uvScale.x) + imageEntry->uvOffset.x;
			vertex->uv.y = (style.xform.transformX(vertex->pos.x, vertex->pos.y) * imageEntry->uvScale.y) + imageEntry->uvOffset.y;
			++vertex;
		}
	}
	else
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

void Geometry::appendVertexBatch(const Vertex* vertices, uint32_t numVertices, const Batch& setup, const RenderStyle* style)
{
	if (!vertices || !numVertices)
		return;

	auto firstVertexIndex = this->vertices.size();
	auto* localVertices = this->vertices.allocateUninitialized(numVertices);
	memcpy(localVertices, vertices, sizeof(Vertex) * numVertices);

	if (style)
		applyStyle(localVertices, numVertices, *style);

	auto& batch = batches.emplaceBack(setup);
	batch.vertexOffset = firstVertexIndex;
}

void Geometry::appendIndexedBatch(const Vertex* vertices, const uint16_t* indices, uint32_t numIndices, const Batch& setup, const RenderStyle* style)
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
	batch.atlasIndex = style ? style->image.atlasIndex : 0;
	batch.vertexOffset = firstVertexIndex;
	batch.vertexCount = numIndices;
}

//--

uint32_t ImageEntry::CalcHash(const ImageEntry& entry)
{
	CRC32 crc;
	crc << entry.atlasIndex;
	crc << entry.entryIndex;
	return crc;
}

//--

END_BOOMER_NAMESPACE(base::canvas)
