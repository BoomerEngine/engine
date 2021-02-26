/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "import_obj_loader_glue.inl"

BEGIN_BOOMER_NAMESPACE_EX(assets)

// parsed OBJ file, RAW data but in memory
// this the "rawest" form of data that is cookable from .OBJ files
class SourceAssetOBJ;
typedef RefPtr<SourceAssetOBJ> SourceAssetOBJPtr;

// parsed MTL file, RAW data but in memory
// this the "rawest" form of data that is cookable from .MTL J files
class SourceAssetMTL;
typedef RefPtr<SourceAssetMTL> SourceAssetMTLPtr;

END_BOOMER_NAMESPACE_EX(assets)
