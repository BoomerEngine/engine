/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game\system #]
***/

#pragma once

#include "gameSystem.h"

namespace game
{
    //--

    /// Resource system for the game - loading/unloading for game related resources
    class GAME_HOST_API GameResourceSystem : public IGameSystem
    {
        RTTI_DECLARE_VIRTUAL_CLASS(GameResourceSystem, IGameSystem);

    public:
        GameResourceSystem();
        virtual ~GameResourceSystem();

        //-- 

        // process mapping file
        void processMappingFile(const game::ResourceDefinitions* resources);

        //--

        // request resource table to be preloaded
        void requestTablePreload(base::StringID name);

        // remove request on resource resource table to be preloaded
        void discardTablePreload(base::StringID name);

        // is resource table preloaded (all resources loaded ?)
        bool tablePreload(base::StringID name) const;

        //--
        
        // get a loaded resource by name, optionally as a total legacy it can be force loaded
        base::res::ResourcePtr acquireResource(base::StringID name, bool forceLoad = false);

        //--

    protected:
        virtual void handleInitialize(IGame* game) override final;
        virtual void handlePreUpdate(IGame* game, double dt) override final;
        virtual void handleDebug(IGame* game) override final;

        struct Entry
        {
            base::StringID fullName; // Weapons.Pistol, etc
            base::res::ResourceKey key; // game::Prefab, "game/weapons/fpp/prefabs/pistol.v4prefab"
            base::res::ResourceWeakPtr loadedRes; // weak pointer to loaded resource
        };

        struct Table
        {
            base::StringID name;
            base::HashMap<base::StringID, Entry*> entries;
        };

        base::HashMap<base::StringID, Table*> m_tableMap;
        base::HashMap<base::StringID, Entry*> m_entryMap;
    };

    //--

} // game
