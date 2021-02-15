/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "import_obj_loader_glue.inl"

namespace wavefront
{

    // parsed OBJ file, RAW data but in memory
    // this the "rawest" form of data that is cookable from .OBJ files
    class FormatOBJ;
    typedef base::RefPtr<FormatOBJ> FormatOBJPtr;

    // parsed MTL file, RAW data but in memory
    // this the "rawest" form of data that is cookable from .MTL J files
    class FormatMTL;
    typedef base::RefPtr<FormatMTL> FormatMTLPtr;

} // wavefront
