/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: format #]
***/

#pragma once

#include "base/resource/include/resource.h"
#include "base/resource_compiler/include/importSourceAsset.h"
#include "base/math/include/vector3.h"

BEGIN_BOOMER_NAMESPACE(assets)

//--

class FormatOBJ;
typedef base::RefPtr<FormatOBJ> FormatOBJPtr;

namespace parser
{
    extern FormatOBJPtr LoadFromBuffer(base::StringView contextName, const void* data, uint64_t dataSize, bool allowThreads);
} // parser

//--

/// attribute types
using Position = base::Vector3;
using Normal = base::Vector3;
using UV = base::Vector2;
using  Color = base::Color;

/// Attribute mask for OBJ vertices
enum AttributeBit : uint8_t
{
    PositionStream = FLAG(0),
    UVStream = FLAG(1),
    NormalStream = FLAG(2),
    ColorStream = FLAG(3), // custom extension, just for testing
};


/// OBJ format face
struct IMPORT_OBJ_LOADER_API Face
{
    uint8_t smoothGroup = 0; // determines if normals can be filtered between faces, usually just 0
    uint8_t numVertices = 0; // in this face, 3,4, etc
    uint8_t attributeMask = 0;
};

/// Material reference
struct IMPORT_OBJ_LOADER_API MaterialRef
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(MaterialRef);

public:
    base::StringBuf name;
};

/// single group chunk that has distinct material
struct IMPORT_OBJ_LOADER_API GroupChunk
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(GroupChunk);

public:
    uint16_t material = 0; // index into the material table
    uint32_t firstFace = 0;
    uint32_t numFaces = 0;
    uint32_t firstFaceIndex = 0;
    uint32_t numFaceIndices = 0;

    uint8_t attributeMask = 0; // used attributes
    uint8_t numAttributes = 0; // number of attributes per vertex in face
    uint8_t commonFaceVertexCount = 0; // 3,4, etc or 0 if faces within this group have different vertex count
};

/// OBJ group
struct IMPORT_OBJ_LOADER_API Group
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Group);

public:
    base::StringBuf name;

    uint32_t firstChunk = 0;
    uint32_t numChunks = 0;
};

/// OBJ object
struct IMPORT_OBJ_LOADER_API Object
{
    RTTI_DECLARE_NONVIRTUAL_CLASS(Object);

public:
    base::StringBuf name;

    uint32_t firstGroup = 0;
    uint32_t numGroups = 0;
};

/// unpacked format data for OBJ file
class IMPORT_OBJ_LOADER_API FormatOBJ : public base::res::ISourceAsset
{
    RTTI_DECLARE_VIRTUAL_CLASS(FormatOBJ, base::res::ISourceAsset);

public:
    FormatOBJ();
    FormatOBJ(const FormatOBJ& other);
        
    //-- vertex streams

    INLINE const Position* positions() const { return (const Position*)m_positions.data(); }
    INLINE const UV* uvs() const { return (const UV*)m_uvs.data(); }
    INLINE const Normal* normals() const { return (const Normal*)m_normals.data(); }
    INLINE const Color* colors() const { return (const Color*)m_colors.data(); }

    INLINE uint32_t numNormals() const { return m_numNormals; }
    INLINE uint32_t numPosition() const { return m_numPositions; }
    INLINE uint32_t numUVS() const { return m_numUVs; }
    INLINE uint32_t numColors() const { return m_numColors; }

    //-- face data

    INLINE const Face* faces() const { return (const Face*)m_faces.data(); }
    INLINE const uint32_t* faceIndices() const { return (const uint32_t*)m_faceIndices.data(); }

    INLINE uint32_t numFaces() const { return m_numFaces; }
    INLINE uint32_t numFaceIndices() const { return m_numFaceIndices; }

    //-- structure

    INLINE const base::Array<GroupChunk>& chunks() const { return m_chunks; }
    INLINE const base::Array<Group>& groups() const { return m_groups; }
    INLINE const base::Array<Object>& objects() const { return m_objects; }

    // -- materials

    INLINE const base::Array<MaterialRef>& materialReferences() const { return m_materialRefs; }
    INLINE const base::StringBuf& materialLibraryFileName() const { return m_matLibFile; }

    //--

        /// calculate estimated memory size of the asset
    virtual uint64_t calcMemoryUsage() const override;

    //--

    // transform all vertices and normals by given matrix
    void transfom(const base::Matrix& globalTransform);

    // transform all vertices and normals (coordinate adjustment)
    void transformSimple(float scale, bool flipYZ);

    // flip winding on all faces
    void flipFaces();

private:
    base::Buffer m_positions;
    base::Buffer m_uvs;
    base::Buffer m_normals;
    base::Buffer m_colors;
    base::Buffer m_faceIndices;
    base::Buffer m_faces;

    uint32_t m_numNormals = 0;
    uint32_t m_numPositions = 0;
    uint32_t m_numUVs = 0;
    uint32_t m_numColors = 0;
    uint32_t m_numFaces = 0;
    uint32_t m_numFaceIndices = 0;

    base::StringBuf m_matLibFile;

    base::Array<MaterialRef> m_materialRefs;
    base::Array<GroupChunk> m_chunks;
    base::Array<Group> m_groups;
    base::Array<Object> m_objects;

    //

    void copy(const FormatOBJ& other);

    friend FormatOBJPtr parser::LoadFromBuffer(base::StringView contextName, const void* data, uint64_t dataSize, bool allowThreads);
};

//--

/// parse OBJ file structure
extern IMPORT_OBJ_LOADER_API FormatOBJPtr LoadObjectFile(base::StringView contextName, const void* data, uint64_t dataSize, bool allowThreads=true);

//--

END_BOOMER_NAMESPACE(assets)
