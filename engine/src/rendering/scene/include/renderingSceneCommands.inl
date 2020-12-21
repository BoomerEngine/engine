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
RENDER_SCENE_COMMAND(SetLocalToWorld)
RENDER_SCENE_COMMAND(SetFlag)
RENDER_SCENE_COMMAND(ClearFlag)

#undef RENDER_SCENE_COMMAND
