/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#include "build.h"
#include "debugGeometry.h"
#include "debugGeometryBuilder.h"
#include "core/memory/include/pageAllocator.h"

BEGIN_BOOMER_NAMESPACE()

//--

DebugGeometryChunk::DebugGeometryChunk()
{}

DebugGeometryChunk::~DebugGeometryChunk()
{}

DebugGeometryChunkPtr DebugGeometryChunk::Create(DebugGeometryLayer layer, uint32_t numVertices, uint32_t numIndices)
{
	auto neededMemory = sizeof(DebugGeometryChunk);
	neededMemory = Align(neededMemory, sizeof(DebugVertex));
	neededMemory += numVertices * sizeof(DebugVertex);
	neededMemory += numIndices * sizeof(uint32_t);

	auto mem = GlobalPool<POOL_DEBUG_GEOMETRY>::Alloc(neededMemory, 16);

	auto* ret = new (mem) DebugGeometryChunk();
	ret->m_layer = layer;
	ret->m_numVertices = numVertices;
    ret->m_numIndices = numIndices;
	ret->m_vertices = AlignPtr((DebugVertex*)(ret + 1), alignof(DebugVertex));
	ret->m_indices = (uint32_t*)(ret->m_vertices + numVertices);

	return NoAddRef(ret);
}

//--

PageAllocator DebugGeometryCollector::st_PageAllocator(POOL_DEBUG_GEOMETRY, 1U << 20, 10, 8);

DebugGeometryCollector::DebugGeometryCollector(uint32_t viewportWidth, uint32_t viewportHight, const Camera& camera)
	: m_allocator(st_PageAllocator)
    , m_viewportWidth(viewportWidth)
	, m_viewportHight(viewportHight)
	, m_camera(camera)
{}

DebugGeometryCollector::~DebugGeometryCollector()
{
	clear();
}

void DebugGeometryCollector::clear()
{
	for (auto& layer : m_layers)
	{
		for (auto* cur = layer.head; cur; cur = cur->next)
			cur->chunk.reset();

		layer = Layer();
	}

	m_allocator.clear();
}

void DebugGeometryCollector::queryLimits(uint32_t& outMaxVertices, uint32_t& outMaxIndices) const
{
	for (const auto& l : m_layers)
	{
		outMaxVertices = std::max<uint32_t>(outMaxVertices, l.totalVertices);
		outMaxIndices = std::max<uint32_t>(outMaxIndices, l.totalIndices);
	}
}

void DebugGeometryCollector::get(DebugGeometryLayer layer, const Element*& outElement, uint32_t& outCount, uint32_t& outNumVertices, uint32_t& outNumIndices) const
{
	const auto& l = m_layers[(int)layer];
	outElement = l.head;
	outCount = l.count;
	outNumVertices = l.totalVertices;
	outNumIndices = l.totalIndices;
}

void DebugGeometryCollector::push(const DebugGeometryChunk* chunk, const Matrix& placement /*= Matrix::IDENTITY()*/, const Selectable& selectable /*= Selectable()*/)
{
	DEBUG_CHECK_RETURN(chunk);

	auto* elem = m_allocator.createNoCleanup<Element>();
	elem->chunk = AddRef(chunk);
	elem->localToWorld = placement;
	elem->selectableOverride = selectable;
	elem->next = nullptr;

	auto& layer = m_layers[(int)chunk->layer()];
	if (!layer.head)
		layer.head = elem;
	if (layer.tail)
		layer.tail->next = elem;
	layer.tail = elem;

	layer.count += 1;
	layer.totalIndices += chunk->numIndices();
	layer.totalVertices += chunk->numVertices();
}

void DebugGeometryCollector::push(const DebugGeometryBuilderBase& data, const Matrix& placement /*= Matrix::IDENTITY()*/, const Selectable& selectable /*= Selectable()*/)
{
	if (data.vertexCount() && data.indexCount())
	{
		auto* chunk = new (m_allocator.alloc(sizeof(DebugGeometryChunk), alignof(DebugGeometryChunk))) DebugGeometryChunk();

		chunk->m_layer = data.layer();
		chunk->m_numVertices = data.vertexCount();
		chunk->m_numIndices = data.indexCount();

		chunk->m_vertices = (DebugVertex*)m_allocator.alloc(sizeof(DebugVertex) * chunk->m_numVertices, alignof(DebugVertex));
		memcpy(chunk->m_vertices, data.vertices(), sizeof(DebugVertex) * chunk->m_numVertices);

		chunk->m_indices = (uint32_t*)m_allocator.alloc(sizeof(uint32_t) * chunk->m_numIndices, alignof(uint32_t));
		memcpy(chunk->m_indices, data.indices(), sizeof(uint32_t) * chunk->m_numIndices);

		push(chunk, placement, selectable);
	}
}

//--

ConfigProperty<float> cvDebugGeometryMaxScreenProjectionDistance("DebugGeometry", "MaxScreenProjectionDistance", 20.0f);
ConfigProperty<float> cvDebugGeometryScreenProjectionBlendStart("DebugGeometry", "ProjectionBlendStart", 0.8f); // fade last 20%

bool DebugGeometryCollector::worldToScreen(const Vector3& pos, Point& outPos, float* outAlpha /*= nullptr*/, float blendDistance /*= 0.0f*/) const
{
    Vector3 screenPos;
    if (!m_camera.projectWorldToScreen(pos, screenPos))
        return false;

	if (blendDistance == 0.0f)
		blendDistance = cvDebugGeometryMaxScreenProjectionDistance.get();

	if (blendDistance > 0.0f)
	{
		const auto dist = m_camera.position().distance(pos);
		if (dist >= blendDistance)
			return false;

		if (outAlpha)
		{
			auto blendStart = blendDistance * cvDebugGeometryScreenProjectionBlendStart.get();
			auto blendRange = std::max<float>(0.001f, blendDistance - blendStart);
			*outAlpha = 1.0f - std::clamp<float>((dist - blendStart) / blendRange, 0.0f, 1.0f);
		}
	}

	outPos.x = screenPos.x * m_viewportWidth;
	outPos.y = screenPos.y * m_viewportHight;
	return true;
}

//--


END_BOOMER_NAMESPACE()
