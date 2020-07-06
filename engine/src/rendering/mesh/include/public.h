/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "rendering_mesh_glue.inl"

namespace rendering
{

    //--

    typedef uint16_t MeshChunkRenderID;

    struct MeshChunk;
    struct MeshMaterial;

    class Mesh;
    typedef base::RefPtr<Mesh> MeshPtr;
    typedef base::res::Ref<Mesh> MeshRef;
    typedef base::res::AsyncRef<Mesh> MeshAsyncRef;

    class MeshService;

    //--

} // rendering

