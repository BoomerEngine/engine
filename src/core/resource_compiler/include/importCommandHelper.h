/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE_EX(res)

//--

/// all-in-one import helper - import files from given list into the depot, runs internally on a fiber, can be canceled via mainProgress
extern CORE_RESOURCE_COMPILER_API bool ProcessImport(const ImportList* files, IProgressTracker* mainProgress, IImportQueueCallbacks* callbacks, bool force = false);

//--

END_BOOMER_NAMESPACE_EX(res)
