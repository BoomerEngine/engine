/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: scene #]
***/

#ifndef RENDER_SCENE_COMMAND
#define RENDER_SCENE_COMMAND(x) x,
#endif

RENDER_SCENE_COMMAND(AttachObject)
RENDER_SCENE_COMMAND(DetachObject)
RENDER_SCENE_COMMAND(MoveObject)
RENDER_SCENE_COMMAND(ChangeFlags)

#undef RENDER_SCENE_COMMAND
