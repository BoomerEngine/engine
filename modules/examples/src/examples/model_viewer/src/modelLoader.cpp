/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: viewer #]
***/

#include "build.h"
#include "modelLoader.h"

namespace viewer
{
    //--

    base::RefPtr<LoadedModel> LoadModel(const base::StringBuf& path)
    {
        base::ScopeTimer timer;

        TRACE_INFO("Loading '{}'...", path);

        // load content of file into memory buffer
        auto fileContent = base::io::OpenMemoryMappedForReading(path);
        if (!fileContent)
        {
            TRACE_ERROR("Unable to open file '{}'", path);
            return nullptr;
        }

        // parse file content
        const auto contextName = StringBuf(path.c_str());
        const auto data = wavefront::LoadObjectFile(contextName, fileContent.data(), fileContent.size());
        if (!data)
        {
            TRACE_ERROR("Loading file '{}' failed", path);
            return nullptr;
        }

        // release source data
        fileContent.reset();

        TRACE_INFO("Loaded '{}':", path);
        TRACE_INFO(" {} objects", data->objects().size());
        TRACE_INFO(" {} groups", data->groups().size());
        TRACE_INFO(" {} chunks", data->chunks().size());
        TRACE_INFO(" {} normals", data->numNormals());
        TRACE_INFO(" {} positions", data->numPosition());
        TRACE_INFO(" {} uvs", data->numUVS());
        TRACE_INFO(" {} faces", data->numFaces());
        TRACE_INFO(" {} faceIndices", data->numFaceIndices());

        // create a rendering data for each chunk

        auto ret = base::RefNew<LoadedModel>();

        return ret;
    }

    //--

} // viewer
