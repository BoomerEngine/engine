/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: import #]
***/

#pragma once

namespace base
{
    namespace res
    {
        //--

        /// all-in-one import helper - import files from given list into the depot, runs internally on a fiber, can be canceled via mainProgress
        extern BASE_RESOURCE_COMPILER_API bool ProcessImport(const ImportList* files, IProgressTracker* mainProgress, IImportQueueCallbacks* callbacks);

        //--

    } // res
} // base