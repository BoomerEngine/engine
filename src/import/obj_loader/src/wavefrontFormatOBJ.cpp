/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: format #]
***/

#include "build.h"
#include "wavefrontFormatOBJ.h"
#include "base/resource/include/resourceTags.h"

namespace wavefront
{

    //--

    RTTI_BEGIN_TYPE_STRUCT(GroupChunk);
        RTTI_PROPERTY(material);
        RTTI_PROPERTY(firstFace);
        RTTI_PROPERTY(numFaces);
        RTTI_PROPERTY(firstFaceIndex);
        RTTI_PROPERTY(numFaceIndices);
        RTTI_PROPERTY(attributeMask);
        RTTI_PROPERTY(numAttributes);
        RTTI_PROPERTY(commonFaceVertexCount);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_STRUCT(Group);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(firstChunk);
        RTTI_PROPERTY(numChunks);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_STRUCT(Object);
        RTTI_PROPERTY(name);
        RTTI_PROPERTY(firstGroup);
        RTTI_PROPERTY(numGroups);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_CLASS(FormatOBJ);
        RTTI_PROPERTY(m_positions);
        RTTI_PROPERTY(m_uvs);
        RTTI_PROPERTY(m_normals);
        RTTI_PROPERTY(m_colors);
        RTTI_PROPERTY(m_faceIndices);
        RTTI_PROPERTY(m_faces);
        RTTI_PROPERTY(m_numNormals);
        RTTI_PROPERTY(m_numPositions);
        RTTI_PROPERTY(m_numUVs);
        RTTI_PROPERTY(m_numColors);
        RTTI_PROPERTY(m_numFaces);
        RTTI_PROPERTY(m_numFaceIndices);
        RTTI_PROPERTY(m_matLibFile);
        RTTI_PROPERTY(m_materialRefs);
        RTTI_PROPERTY(m_chunks);
        RTTI_PROPERTY(m_groups);
        RTTI_PROPERTY(m_objects);
    RTTI_END_TYPE();

    RTTI_BEGIN_TYPE_STRUCT(MaterialRef);
        RTTI_PROPERTY(name);
    RTTI_END_TYPE();

    //--

    FormatOBJ::FormatOBJ()
    {}

    FormatOBJ::FormatOBJ(const FormatOBJ& other)
    {
        copy(other);
    }

    void FormatOBJ::copy(const FormatOBJ& other)
    {
        m_positions = other.m_positions;
        m_uvs = other.m_uvs;
        m_normals = other.m_normals;
        m_colors = other.m_colors;
        m_faceIndices = other.m_faceIndices;
        m_faces = other.m_faces;

        m_numNormals = other.m_numNormals;
        m_numPositions = other.m_numPositions;
        m_numUVs = other.m_numUVs;
        m_numColors = other.m_numColors;
        m_numFaces = other.m_numFaces;
        m_numFaceIndices = other.m_numFaceIndices;
        m_matLibFile = other.m_matLibFile;

        m_materialRefs = other.m_materialRefs;
        m_chunks = other.m_chunks;
        m_groups = other.m_groups;
        m_objects = other.m_objects;
    }

    uint64_t FormatOBJ::calcMemoryUsage() const
    {
        uint64_t totalSize = 0;
        totalSize += m_positions.size();
        totalSize += m_uvs.size();
        totalSize += m_normals.size();
        totalSize += m_colors.size();
        totalSize += m_faces.size();
        totalSize += m_faceIndices.size();
        totalSize += m_chunks.dataSize();
        totalSize += m_groups.dataSize();
        totalSize += m_objects.dataSize();
        return totalSize;
    }

    void FormatOBJ::transformSimple(float scale, bool flipYZ)
    {
        auto v  = (Position*)m_positions.data();
        auto endV  = v + m_numPositions;

        if (flipYZ)
        {
            while (v < endV)
                *v++ *= scale;
        }
        else
        {
            while (v < endV)
            {
                std::swap(v->y, v->z);
                *v++ *= scale;
            }
        }

        if (flipYZ)
            flipFaces();
    }

    void FormatOBJ::transfom(const base::Matrix& globalTransform)
    {
        {
            auto v  = (Position*)m_positions.data();
            auto endV  = v + m_numPositions;

            while (v < endV)
            {
                *v = globalTransform.transformPoint(*v);
                ++v;
            }
        }

        {
            auto n  = (Normal*)m_positions.data();
            auto endN  = n + m_numNormals;

            while (n < endN)
            {
                *n = globalTransform.transformVector(*n);
                ++n;
            }
        }
    }

    void FormatOBJ::flipFaces()
    {
        auto f  = (Face*)m_faces.data();
        auto endF  = f + m_numFaces;

        auto fi  = (uint32_t*)m_faceIndices.data();
        while (f < endF)
        {
            auto i  = fi;
            auto ei  = i + f->numVertices - 1;

            while (i < ei)
                std::swap(*i++, *ei++);

            fi += f->numVertices;
            ++f;
        }
    }

    ///--

    /// source asset loader for OBJ data
    class FormatOBJAssetLoader : public base::res::ISourceAssetLoader
    {
        RTTI_DECLARE_VIRTUAL_CLASS(FormatOBJAssetLoader, base::res::ISourceAssetLoader);

    public:
        virtual base::res::SourceAssetPtr loadFromMemory(base::StringView importPath, base::StringView contextPath, base::Buffer data) const override
        {
            return LoadObjectFile(contextPath, data.data(), data.size(), true);
        }
    };

    RTTI_BEGIN_TYPE_CLASS(FormatOBJAssetLoader);
        RTTI_METADATA(base::res::ResourceSourceFormatMetadata).addSourceExtensions("obj");
    RTTI_END_TYPE();

    ///--

} // wavefront

