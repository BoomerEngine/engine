/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: config #]
***/

#include "build.h"
#include "configEntry.h"
#include "configGroup.h"
#include "configStorage.h"
#include "configSystem.h"

base::mem::PoolID POOL_CONFIG("Engine.Config");

namespace base
{
    namespace config
    {

        System::System()
        {
            m_storage = MemNewPool(POOL_CONFIG, Storage);
        }

        void System::deinit()
        {
            MemDelete(m_storage);
            m_storage = nullptr;
        }

        Group& System::group(StringID name)
        {
            return m_storage->group(name);
        }

        const Group* System::findGroup(StringID name) const
        {
            return m_storage->findGroup(name);
        }

        Array<const Group*> System::findAllGroups(StringView<char> groupNameSubString) const
        {
            return m_storage->findAllGroups(groupNameSubString);
        }

        Entry& System::entry(StringID groupName, StringID entryName)
        {
            return group(groupName).entry(entryName);
        }

        const Entry* System::findEntry(StringID groupName, StringID entryName) const
        {
            if (auto group  = findGroup(groupName))
                return group->findEntry(entryName);

            return nullptr;
        }

        const Array<StringBuf> System::values(StringID groupName, StringID entryName) const
        {
            if (auto entry  = findEntry(groupName, entryName))
                return entry->values();

            return Array<StringBuf>();
        }

        StringBuf System::value(StringID groupName, StringID entryName, const StringBuf& defaultValue) const
        {
            if (auto entry  = findEntry(groupName, entryName))
                return entry->value();

            return defaultValue;
        }

        int System::valueInt(StringID groupName, StringID entryName, int defaultValue/* = 0*/) const
        {
            if (auto entry  = findEntry(groupName, entryName))
                return entry->valueInt(defaultValue);

            return defaultValue;
        }

        float System::valueFloat(StringID groupName, StringID entryName, float defaultValue/* = 0*/) const
        {
            if (auto entry  = findEntry(groupName, entryName))
                return entry->valueInt(defaultValue);

            return defaultValue;
        }

        bool System::valueBool(StringID groupName, StringID entryName, bool defaultValue/*= 0*/) const
        {
            if (auto entry  = findEntry(groupName, entryName))
                return entry->valueInt(defaultValue);

            return defaultValue;
        }

    } // config
} // storage