/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: frame\debug #]
***/

#include "build.h"
#include "renderingFrameDebugGeometry.h"
#include "base/memory/include/pageAllocator.h"

namespace rendering
{
    namespace scene
    {
        //--

        static base::mem::PageAllocator GDebugGeometryPayloadAllocator(POOL_DEBUG_GEOMETRY, 256 << 20, 8, 64);
        static base::mem::PageAllocator GDebugGeometryStructureAllocator(POOL_DEBUG_GEOMETRY, 16 << 10, 64, 64);

        //--

        DebugGeometry::DebugGeometry()
            : m_vertices(GDebugGeometryPayloadAllocator)
            , m_indices(GDebugGeometryPayloadAllocator)
            //, m_elements(GDebugGeometryStructureAllocator)
        {
        }

		void DebugGeometry::clear()
		{
			m_indices.clear();
			m_vertices.clear();
			m_elements.clear();
		}

        void DebugGeometry::push(const DebugGeometryElementSrc& source)
        {
            DEBUG_CHECK_RETURN_EX(source.sourceVerticesData, "No data");

			auto& element = m_elements.emplaceBack();
			element.type = source.type;
			element.firstVertex = m_vertices.size();
			element.numIndices = source.numIndices;
			element.firstIndex = m_indices.size();
			element.numVeritices = source.numIndices;

			if (source.sourceVerticesData)
				m_vertices.writeLarge(source.sourceVerticesData, source.numVeritices * sizeof(DebugVertex));

			if (source.sourceIndicesData)
				m_indices.writeLarge(source.sourceIndicesData, source.numIndices * sizeof(uint32_t));
        }

        //--

    } // debug
} // rendering
