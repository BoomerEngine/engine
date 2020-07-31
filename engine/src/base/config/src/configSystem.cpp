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

        //--

        namespace prv
        {

            class ConfigStorage : public ISingleton
            {
                DECLARE_SINGLETON(ConfigStorage);

            public:
                ConfigStorage()
                {
                    m_storage = MemNewPool(POOL_CONFIG, Storage);
                }

                virtual void deinit() override
                {
                    MemDelete(m_storage);
                }

                Storage* m_storage;
            };
        }

        //--

        Storage& RawStorageData()
        {
            return *prv::ConfigStorage::GetInstance().m_storage;
        }

        //--

        Group& MakeGroup(StringView<char> name)
        {
            return prv::ConfigStorage::GetInstance().m_storage->group(name);
        }

        const Group* FindGroup(StringView<char> name)
        {
            return prv::ConfigStorage::GetInstance().m_storage->findGroup(name);
        }

        Array<const Group*> FindAllGroups(StringView<char> groupNameSubString)
        {
            return prv::ConfigStorage::GetInstance().m_storage->findAllGroups(groupNameSubString);
        }

        Entry& MakeEntry(StringView<char> groupName, StringView<char> entryName)
        {
            return MakeGroup(groupName).entry(entryName);
        }

        const Entry* FindEntry(StringView<char> groupName, StringView<char> entryName)
        {
            if (auto group = FindGroup(groupName))
                return group->findEntry(entryName);

            return nullptr;
        }

        const Array<StringBuf> Values(StringView<char> groupName, StringView<char> entryName)
        {
            if (auto entry = FindEntry(groupName, entryName))
                return entry->values();

            return Array<StringBuf>();
        }

        StringBuf Value(StringView<char> groupName, StringView<char> entryName, const StringBuf& defaultValue)
        {
            if (auto entry = FindEntry(groupName, entryName))
                return entry->value();

            return defaultValue;
        }

        int ValueInt(StringView<char> groupName, StringView<char> entryName, int defaultValue/* = 0*/)
        {
            if (auto entry = FindEntry(groupName, entryName))
                return entry->valueInt(defaultValue);

            return defaultValue;
        }

        float ValueFloat(StringView<char> groupName, StringView<char> entryName, float defaultValue/* = 0*/)
        {
            if (auto entry = FindEntry(groupName, entryName))
                return entry->valueInt(defaultValue);

            return defaultValue;
        }

        bool ValueBool(StringView<char> groupName, StringView<char> entryName, bool defaultValue/*= 0*/)
        {
            if (auto entry = FindEntry(groupName, entryName))
                return entry->valueBool(defaultValue);

            return defaultValue;
        }

        void Write(StringView<char> groupName, StringView<char> entryName, StringView<char> value)
        {
            if (groupName && entryName)
            {
                auto& data = MakeEntry(groupName, entryName);
                data.value(StringBuf(value));
            }
        }

        void WriteInt(StringView<char> groupName, StringView<char> entryName, bool value)
        {
            if (groupName && entryName)
            {
                auto& data = MakeEntry(groupName, entryName);
                data.value(TempString("{}", value));
            }
        }

        void WriteBool(StringView<char> groupName, StringView<char> entryName, bool value)
        {
            if (groupName && entryName)
            {
                auto& data = MakeEntry(groupName, entryName);
                data.value(value ? "true" : "false");
            }
        }

        //--

    } // config
} // storage