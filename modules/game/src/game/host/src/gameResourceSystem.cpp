/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: game #]
***/

#include "build.h"
#include "gameResourceSystem.h"
#include "gameResourceDefinitions.h"

namespace game
{
    //----

    RTTI_BEGIN_TYPE_CLASS(GameResourceSystem);
    RTTI_END_TYPE();

    //---

    GameResourceSystem::GameResourceSystem()
    {}

    GameResourceSystem::~GameResourceSystem()
    {
        m_tableMap.clearPtr();
        m_entryMap.clearPtr();
    }

    void GameResourceSystem::requestTablePreload(base::StringID name)
    {
        // TODO
    }

    void GameResourceSystem::discardTablePreload(base::StringID name)
    {
        // TODO
    }

    bool GameResourceSystem::tablePreload(base::StringID name) const
    {
        return true;
    }

    void GameResourceSystem::processMappingFile(const game::ResourceDefinitions* resources)
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

    base::res::ResourcePtr GameResourceSystem::acquireResource(base::StringID name, bool forceLoad /*= false*/)
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

    static base::res::StaticResource<game::ResourceDefinitions> cvResCommonResources("/game/common_resources.v4reslist");

    void GameResourceSystem::handleInitialize(IGame* game)
    {
        processMappingFile(cvResCommonResources.loadAndGet());
    }

    void GameResourceSystem::handlePreUpdate(IGame* game, double dt)
    {
        // TODO: update loading
    }

    void GameResourceSystem::handleDebug(IGame* game)
    {

    }

    //--

} // ui
