/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: example #]
***/

#pragma once

#include "gameScene.h"

namespace example
{
    //--

    // render "sky" effect
    extern void RenderSky(CommandWriter& cmd, uint32_t width, uint32_t height, Vector2 center, float time);

    //--

} // example

