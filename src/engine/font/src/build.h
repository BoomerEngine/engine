/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "public.h"

// FreeType integration
// NOTE: it's not public, we don't want to leak symbols outside
extern "C"
{
    #include <ft2build.h>
    #include FT_FREETYPE_H
}

