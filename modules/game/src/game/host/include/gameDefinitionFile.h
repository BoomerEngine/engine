/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: resources #]
***/

#pragma once

#include "base/containers/include/hashMap.h"

namespace game
{

    //---

    /// game definition file
    class GAME_HOST_API DefinitionFile : public base::res::IResource
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DefinitionFile, base::res::IResource);

    public:
        DefinitionFile();

        //--

        base::Array<ResourceDefinitionsRef> m_resources;

        GameLauncherPtr m_standaloneLauncher; // used when running a standalone game
        GameLauncherPtr m_editorLauncher; // used when running game from an editor

        //--
    };

    //---

} // game