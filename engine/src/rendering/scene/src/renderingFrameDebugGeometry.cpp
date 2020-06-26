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

        static base::mem::PoolID POOL_DEBUG_GEOMETRY("Rendering.DebugGeometry");

        static base::mem::PageAllocator GDebugGeometryPayloadAllocator(POOL_DEBUG_GEOMETRY, 256 << 20, 8, 64);
        static base::mem::PageAllocator GDebugGeometrySmallPayloadAllocator(POOL_DEBUG_GEOMETRY, 64 << 10, 16, 256);
        static base::mem::PageAllocator GDebugGeometryStructureAllocator(POOL_DEBUG_GEOMETRY, 16 << 10, 64, 64);

        //--

        DebugGeometry::DebugGeometry(DebugGeometryLayer layer)
            : m_verticesData((layer == DebugGeometryLayer::SceneSolid) ? GDebugGeometryPayloadAllocator : GDebugGeometrySmallPayloadAllocator)
            , m_verticesDataEx((layer == DebugGeometryLayer::SceneSolid) ? GDebugGeometryPayloadAllocator : GDebugGeometrySmallPayloadAllocator)
            , m_indicesData((layer == DebugGeometryLayer::SceneSolid) ? GDebugGeometryPayloadAllocator : GDebugGeometrySmallPayloadAllocator)
            , m_elements(GDebugGeometryStructureAllocator)
            , m_layer(layer)
        {
        }

        DebugGeometry::~DebugGeometry()
        {
        }

        void DebugGeometry::pushElement(const DebugGeometryElementSrc& source, const DebugVertexEx& defaultDebugVertex)
        {
            DEBUG_CHECK(source.sourceIndicesData && source.sourceVerticesData);

            if (source.sourceIndicesData && source.sourceVerticesData)
            {
                const auto numIndices = source.sourceIndicesData->size();
                const auto numVertices = source.sourceVerticesData->size();
                if (numIndices && numVertices)
                {
                    auto* element = m_elements.allocSingle();
                    element->type = source.type;
                    element->firstVertex = m_verticesData.size();
                    element->numIndices = numIndices;
                    element->firstIndex = m_indicesData.size();
                    element->numVeritices = numVertices;

                    m_verticesData.append(*source.sourceVerticesData);

                    DEBUG_CHECK_EX(!source.sourceVerticesDataEx || source.sourceVerticesDataEx->size() == numVertices, "Invalid data count for secondary vertex stream");
                    if (source.sourceVerticesDataEx && source.sourceVerticesDataEx->size() == numVertices)
                        m_verticesDataEx.append(*source.sourceVerticesDataEx);
                    else
                        m_verticesDataEx.fill(&defaultDebugVertex, numVertices);

                    {
                        auto vertexOffset = element->firstVertex;
                        source.sourceIndicesData->forEach([vertexOffset](const uint32_t& index)
                            {
                                const_cast<uint32_t&>(index) += vertexOffset;
                            });
                    }

                    m_indicesData.append(*source.sourceIndicesData);
                }
            }
        }

        //--

    } // debug
} // rendering
