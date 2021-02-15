/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "import_fbx_loader_glue.inl"

namespace fbxsdk
{

    class FbxNode;
    class FbxManager;
    class FbxScene;
    class FbxMesh;
    class FbxSurfaceMaterial;
    class FbxNode;

} // fbxsdk

namespace fbx
{
    class LoadedFile;
    typedef base::RefPtr<LoadedFile> LoadedFilePtr;

} // fbx