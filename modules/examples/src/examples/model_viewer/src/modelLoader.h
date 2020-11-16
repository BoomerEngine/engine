/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: viewer #]
***/

#pragma once

namespace viewer
{
#if 0
    //--

    // model material information

    // Rendering chunk for model
    struct ModelChunk
    {
        uint32_t m_firstVertex;
        uint32_t m_firstIndex;
        uint32_t m_numIndices;
        uint32_t m_numVertices;
        uint32_t m_materialIndex;
    };

    // Rendering data for model, based from engine mesh
    class Model : public res::IResource, public IDeviceObject
    {
    public:
        Model();
        Model(const mesh::MeshPtr& sourcMesh);

        // render model
        void render(rendering::command::CommandWriter& cmd, const Matrix& localToWorld) const;

    protected:
        virtual void handleDeviceReset() override final;
        virtual void handleDeviceRelease() override final;


        //--
        
        BufferView m_vertexData;
        BufferView m_indexData;
    };

    //--
#endif

    // merged vertex
    struct LoadedVertex
    {
        Vector3 pos;
        Vector3 normal;
        Vector2 uv;
    };

    // loaded texture data
    struct LoadedTexture
    {
        ImageView m_loadedTexture;
    };

    // loaded material
    struct LoadedMaterial
    {
        StringID name;
        RefPtr<LoadedTexture> diffuseMap;
        Color diffuseColor = Color::WHITE;
        bool masked = false;
    };

    // loaded model chunk
    struct LoadedChunk
    {
        RefPtr<LoadedMaterial> material;
        Buffer vertices;
        Buffer indices;

        uint32_t numVertices = 0;
        uint32_t numIndices = 0;
    };

    // material for rendering
    struct RenderMaterial
    {
        bool masked = false;
        float maskThreshold = 0.5f;
        ImageView diffuseTexture;
        Vector4 diffuseColor;
    };

    // chunk for rendering
    struct RenderChunk
    {
        BufferView vertices;
        BufferView indices;
    };

    // model we can render
    class LoadedModel : public NoCopy
    {
    public:
        Array<LoadedMaterial> materials;
        Array<LoadedChunk> chunks;
    };

    //--

    // load OBJ model from given absolute path
    extern RefPtr<LoadedModel> LoadModel(const StringBuf& path);

    //--

} // viewer

