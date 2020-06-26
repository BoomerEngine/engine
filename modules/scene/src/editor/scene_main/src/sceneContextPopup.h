/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: actions #]
***/

#pragma once

#include "scene/common/include/sceneNodeTemplate.h"
#include "scene/common/include/scenePrefab.h"

namespace ed
{
    namespace world
    {

        ///--

        class SelectionContext;

        /// build popup menu for selection context
        extern EDITOR_SCENE_MAIN_API ui::PopupMenuPtr BuildPopupMenu(const base::RefPtr<SelectionContext>& selection);

        ///--

    } // world
} // ed
