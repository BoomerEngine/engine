/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "gameResourceSystem.h"
#include "gameResourceMapping.h"

namespace game
{
    //----

    RTTI_BEGIN_TYPE_CLASS(GameSystem_Resources);
    RTTI_END_TYPE();

    //---

    GameSystem_Resources::GameSystem_Resources()
    {}

    GameSystem_Resources::~GameSystem_Resources()
    {
        m_tableMap.clearPtr();
        m_entryMap.clearPtr();
    }

    void GameSystem_Resources::requestTablePreload(base::StringID name)
    {
        // TODO
    }

    void GameSystem_Resources::discardTablePreload(base::StringID name)
    {
        // TODO
    }

    bool GameSystem_Resources::tablePreload(base::StringID name) const
    {
        return true;
    }

    void GameSystem_Resources::processMappingFile(const game::ResourceList* resources)
    {
        if (!resources)
            return;

        // insert resources
        for (const auto& table : resources->tables())
        {
            // get table
            Table* localTable = nullptr;
            m_tableMap.find(table.name, localTable);
            if (!localTable)
            {
                localTable = MemNew(Table).ptr;
                localTable->name = table.name;
                m_tableMap[table.name] = localTable;
            }

            // insert resource entries
            for (const auto& entry : table.resources)
            {
                Entry* localEntry = nullptr;
                localTable->entries.find(entry.name, localEntry);
                if (nullptr == localEntry)
                {
                    localEntry = MemNew(Entry).ptr;
                    localEntry->fullName = base::StringID(base::TempString("{}.{}", table.name, entry.name));
                    localEntry->key = entry.resource.key();
                    
                    localTable->entries[entry.name] = localEntry;
                    m_entryMap[localEntry->fullName] = localEntry;

                    TRACE_INFO("Found game resource '{}': '{}'", localEntry->fullName, localEntry->key);
                }
                else
                {
                    if (localEntry->key != entry.resource.key())
                        TRACE_WARNING("Resource entry '{}' in table '{}' has conflicting definition: '{}' and '{}'", entry.name, table.name, entry.resource.key(), localEntry->key);
                }
            }
        }
    }

    base::res::ResourcePtr GameSystem_Resources::acquireResource(base::StringID name, bool forceLoad /*= false*/)
    {
        if (!name)
            return nullptr;

        // find in the map first
        Entry* entry = nullptr;
        if (!m_entryMap.find(name, entry))
        {
            TRACE_WARNING("Missing game resource entry '{}'", name);
            return nullptr;
        }

        // if already loaded use it directly
        if (auto loaded = entry->loadedRes.lock())
            return loaded;

        // load
        // TODO: rest of the async stuff

        auto loaded = base::LoadResource(entry->key).acquire();
        entry->loadedRes = loaded;
        return loaded;
    }

    //--

    static base::res::StaticResource<game::ResourceList> cvResCommonResources("game/common_resources.v4reslist");

    void GameSystem_Resources::handleInitialize(IGame* game)
    {
        processMappingFile(cvResCommonResources.loadAndGet());
    }

    void GameSystem_Resources::handlePreUpdate(IGame* game, double dt)
    {
        // TODO: update loading
    }

    void GameSystem_Resources::handleDebug(IGame* game)
    {

    }

    //--

} // ui
