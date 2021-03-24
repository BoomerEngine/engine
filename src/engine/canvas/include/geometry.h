/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

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

#pragma once
#include "style.h"

BEGIN_BOOMER_NAMESPACE()

//--

#pragma pack(push)
#pragma pack(4)
struct CanvasAttributes
{
	Vector2 base;
	Vector2 extent;
	Color innerColor;
	Color outerColor;
	float radius = 0.0f;
	float feather = 0.0f;
	float lineWidth = 0.0f;

	//--

	bool operator==(const CanvasAttributes& other) const;
	bool operator!=(const CanvasAttributes& other) const;

	static uint32_t CalcHash(const CanvasAttributes& style);

private:
	uint32_t _padding0 = 0;
};
#pragma pack(pop)

static_assert(sizeof(CanvasAttributes) == 40, "Make sure there are no holes");

//--

/// Simple structure to hold renderable canvas geometry (vertices + draw commands)
/// Can be placed (instanced) in canvas with a custom transformation
/// NOTE: data cached here depends on the particular UVs and atlas placements or images in the storage!
struct ENGINE_CANVAS_API CanvasGeometry
{
    RTTI_DECLARE_POOL(POOL_CANVAS)

public:
    CanvasGeometry();
	CanvasGeometry(const CanvasGeometry& other);
	CanvasGeometry(CanvasGeometry&& other);
	CanvasGeometry& operator=(const CanvasGeometry& other);
	CanvasGeometry& operator=(CanvasGeometry&& other);
	~CanvasGeometry();

    //--

    INLINE bool empty() const { return batches.empty(); }
	INLINE operator bool() const { return !batches.empty(); }

    //--

	// reset geometry without freeing memory
	void reset();

	// count used memory
	uint32_t calcMemorySize() const;

	//--

	Vector2 boundsMin;
	Vector2 boundsMax;

	Array<CanvasVertex> vertices;
	Array<CanvasBatch> batches;
	Array<CanvasAttributes> attributes;
	Array<uint8_t> customData;

	//--
			
	// directly append a batch to the geometry
	void appendVertexBatch(const CanvasVertex* vertices, uint32_t numVertices, const CanvasBatch& setup = CanvasBatch(), const CanvasRenderStyle* style = nullptr);

	// directly append a batch to the geometry indexed triangle list batch to the geometry
	void appendIndexedBatch(const CanvasVertex* vertices, const uint16_t* indices, uint32_t numIndices, const CanvasBatch& setup = CanvasBatch(), const CanvasRenderStyle* style = nullptr);

	//--

private:
	int appendStyle(const CanvasRenderStyle& style);
	void applyStyle(CanvasVertex* vertices, uint32_t numVertices, const CanvasRenderStyle& style);
};

//--

END_BOOMER_NAMESPACE()
