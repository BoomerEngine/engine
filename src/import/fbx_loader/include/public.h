/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "import_fbx_loader_glue.inl"

namespace ofbx
{
    struct IScene;
    struct Material;
    struct Mesh;
    struct Object;
} // ofbx

BEGIN_BOOMER_NAMESPACE_EX(assets)

class FBXFile;
typedef RefPtr<FBXFile> FBXFilePtr;

END_BOOMER_NAMESPACE_EX(assets)
