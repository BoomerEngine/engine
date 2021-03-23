/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: compiler #]
*
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

//---

/// compile given depot world
extern ENGINE_WORLD_COMPILER_API CompiledWorldDataPtr CompileRawWorld(StringView depotPath, IProgressTracker& progress);

//---

END_BOOMER_NAMESPACE()
