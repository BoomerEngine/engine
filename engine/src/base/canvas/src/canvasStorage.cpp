/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: canvas #]
***/

#include "build.h"

#include "canvasGeometry.h"
#include "canvasGeometryBuilder.h"
#include "canvasStorage.h"

namespace base
{
    namespace canvas
    {

		//--

		BakedGeometry::BakedGeometry(base::Array<Vertex>&& vertices, base::Array<Batch>&& batches, const Vector2& minBounds, const Vector2& maxBounds)
			: m_vertices(std::move(vertices))
			, m_batches(std::move(batches))
			, m_vertexBoundsMin(minBounds)
			, m_vertexBoundsMax(maxBounds)
		{}

		BakedGeometry::~BakedGeometry()
		{}

        //---

		RTTI_BEGIN_TYPE_ABSTRACT_CLASS(IStorage);
		RTTI_END_TYPE();

		IStorage::IStorage()
		{
		}

		IStorage::~IStorage()
		{
		}

		//--
        
    } // canvas
} // base