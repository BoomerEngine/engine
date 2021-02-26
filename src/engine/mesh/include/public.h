/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "engine_mesh_glue.inl"

BEGIN_BOOMER_NAMESPACE()

//--

/// chunk rendering mask, defines when given chunk is rendered
enum class MeshChunkRenderingMaskBit : uint32_t
{
    Scene = FLAG(0), // chunk is used then rendering visible geometry
    ObjectShadows = FLAG(1), // chunk is used when rendering hi-res shadows for the object
    LocalShadows = FLAG(2), // chunk is used when rendering local shadows (points lights, spot lights, etc)
    Cascade0 = FLAG(3), // chunk is used for rendering shadows in cascade 0
    Cascade1 = FLAG(4), // chunk is used for rendering shadows in cascade 1
    Cascade2 = FLAG(5), // chunk is used for rendering shadows in cascade 2
    Cascade3 = FLAG(6), // chunk is used for rendering shadows in cascade 3
    LodMerge = FLAG(7), // use this chunk when generating merged lod
    ShadowMesh = FLAG(8), // this chunk is part of global shadow mesh
    LocalReflection = FLAG(9), // render this chunk in local reflection probes
    GlobalReflection = FLAG(10), // render this chunk in global reflection probes
    Lighting = FLAG(11), // include this chunk in lighting computations
    StaticOccluder = FLAG(12), // include this chunk in static occlusion calculations
    DynamicOccluder = FLAG(13), // include this chunk in dynamic occlusion calculations
    ConvexCollision = FLAG(14), // convex collision data (can be shared with rendering as well)
    ExactCollision = FLAG(15), // exact collision data
    Cloth = FLAG(16), // cloth mesh

    DEFAULT = Scene | ObjectShadows | LocalShadows | Cascade0 | Cascade1 | Cascade2 | Cascade3 | ShadowMesh | LodMerge | LocalReflection | GlobalReflection | Lighting | StaticOccluder,
    RENDERABLE = Scene | ObjectShadows | LocalShadows | Cascade0 | Cascade1 | Cascade2 | Cascade3 | ShadowMesh | LocalReflection | GlobalReflection | Lighting,
    ALL = 0xFFFF,
};

typedef DirectFlags<MeshChunkRenderingMaskBit> MeshChunkRenderingMask;

//--

// general material flag
enum class MeshMaterialFlagBit : uint16_t
{
    Unlit = FLAG(0), // material does not want to receive lighting
    Simple = FLAG(1), // material does not implement the full PBR pipeline
    Transparent = FLAG(2), // material is transparent
    Masked = FLAG(3), // material is masked
    Undefined = FLAG(4), // we don't have data for this material
};

typedef DirectFlags<MeshMaterialFlagBit> MeshMaterialFlags;

//--

/// mesh chunk topology type
enum class MeshTopologyType : uint8_t
{
    Triangles,
    Quads,
};

//--

enum class MeshServiceTier : uint8_t
{
	SeparateBuffers, // each chunk is in separate VB/IB
	Bindless, // chunks are managed in one buffer
};

//--

struct MeshChunk;
struct MeshMaterial;

typedef uint16_t MeshChunkID;

class MeshRenderChunkPayload;
typedef RefPtr<MeshRenderChunkPayload> MeshRenderChunkPayloadPtr;

class MeshMaterialsPayload;
typedef RefPtr<MeshMaterialsPayload> MeshMaterialsPayloadPtr;

class MeshService;

class Mesh;
typedef RefPtr<Mesh> MeshPtr;
typedef res::Ref<Mesh> MeshRef;
typedef res::AsyncRef<Mesh> MeshAsyncRef;

class IMeshChunkProxy;
typedef RefPtr<IMeshChunkProxy> MeshChunkProxyPtr;

class MeshChunkProxy_Meshlets;
typedef RefPtr<MeshChunkProxy_Meshlets> MeshChunkProxyMeshletsPtr;

class MeshChunkProxy_Standalone;
typedef RefPtr<MeshChunkProxy_Standalone> MeshChunkProxyStandalonePtr;

//--

END_BOOMER_NAMESPACE()

